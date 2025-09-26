/* Evoution On plugin
 *  Copyright (C) 2008-2012 Lucian Langa <cooly@gnome.eu.org>
 *  Copyright (C) 2022 Ozan Türkyılmaz <ozan.turkyilmaz@gmail.com>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <string.h>

#ifndef G_OS_WIN32
#include <gdk/gdkx.h> 
#endif

#include <e-util/e-util.h>

#include <shell/e-shell.h>
#include <shell/e-shell-view.h>
#include <shell/e-shell-window.h>

#include <libemail-engine/libemail-engine.h>

#include <mail/em-event.h>
#include <mail/em-folder-tree.h>

#include <mail/e-mail-reader.h>

#ifdef DEBUG
#include <glib/gprintf.h>
#endif

#include "on_properties.h"
#include "on_icon.h"

static gulong show_window_handle = 0;
static gboolean show_window_cb_called = FALSE;
struct OnIcon on_icon = ONICON_NEW;

//helper method for toggling used on init for hidden on startup and on tray click
static void
toggle_window()
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	if (gtk_widget_get_visible(GTK_WIDGET(on_icon.evo_window))) {
		gtk_widget_hide(GTK_WIDGET(on_icon.evo_window));
	} else {
		gtk_widget_show(GTK_WIDGET(on_icon.evo_window));
	}

	if (on_icon.winnotify) {
		set_icon(&on_icon, FALSE, _(""));
		on_icon.winnotify = FALSE;
	}
}

/* Quit the evolution */
static void
do_quit(GtkMenuItem *item, gpointer user_data)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	EShell *shell;
	shell = e_shell_get_default();
	e_shell_quit(shell, E_SHELL_QUIT_ACTION);
}

/* Show and do properties of the plugin */
static void
do_properties(GtkMenuItem *item, gpointer user_data)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	GtkWidget *cfg, *ocfg, *dialog, *label, *vbox, *hbox;
	GtkWidget *content_area;
	gchar *text;

	cfg = get_cfg_widget();
	if (!cfg)
		return;
	ocfg = get_original_cfg_widget();
	if (!ocfg)
		return;

	text = g_markup_printf_escaped("<span size=\"x-large\">%s</span>",
			_("Evolution On"));

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	label = gtk_label_new(NULL);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), text);
	g_free (text);

	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	gtk_widget_show(vbox);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	label = gtk_label_new("   ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show_all(hbox);

	gtk_box_pack_start(GTK_BOX (vbox), cfg, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (vbox), ocfg, TRUE, TRUE, 0);

	dialog = gtk_dialog_new_with_buttons(_("Mail Notification Properties"),
			NULL,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			"window-close", GTK_RESPONSE_CLOSE,
			NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	gtk_container_add(GTK_CONTAINER(content_area), vbox);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), 10);
	gtk_widget_set_size_request(dialog, 400, -1);
	g_signal_connect_swapped(dialog, "response",
			G_CALLBACK(gtk_widget_destroy), dialog);
	gtk_widget_show(dialog);
}

/* Show window when clicked on our icon */
static void
shown_window_cb(GtkWidget *widget, gpointer user_data)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	if (!show_window_cb_called) {
		if (is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDDEN_ON_STARTUP)) {
			on_icon.toggle_window_func();
		}
		show_window_cb_called = TRUE;
	}
}

/* New email notification */
static void
new_notify_status(EMEventTargetFolder *t, struct OnIcon *_onicon)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	gchar *msg;

	EShell *shell = e_shell_get_default();
	CamelStore *store;
	gchar *folder_name;
	EMailBackend *backend;
	EMailSession *session;
	EShellBackend *shell_backend;

	_onicon->uri = g_strdup(t->folder_name);

	ESource *source = NULL;
		ESourceRegistry *registry;
	const gchar *name;

	const gchar *uid;

	uid = camel_service_get_uid(CAMEL_SERVICE(t->store));
	registry = e_shell_get_registry(shell);
	source = e_source_registry_ref_source(registry,uid);
	name = e_source_get_display_name(source);

	shell_backend = e_shell_get_backend_by_name(shell, "mail");

	backend = E_MAIL_BACKEND(shell_backend);
	session = e_mail_backend_get_session(backend);

	e_mail_folder_uri_parse (CAMEL_SESSION(session), t->folder_name,
			&store, &folder_name, NULL);

	if (name != NULL)
		folder_name = g_strdup_printf("%s/%s", name, folder_name);
	else
		folder_name = g_strdup(folder_name);

	_onicon->status_count = t->new;

	/* Translators: '%d' is the count of mails received
	 * and '%s' is the name of the folder
	 */
	msg = g_strdup_printf(ngettext(
			"You have received %d new message\nin %s.",
		"	You have received %d new messages\nin %s.",
			_onicon->status_count), _onicon->status_count, folder_name);

	g_free(folder_name);
	if (t->msg_sender) {
		gchar *tmp, *str;

		/* Translators: "From:" is preceding a new mail
		 * sender address, like "From: user@example.com"
		 */
		str = g_strdup_printf(_("From: %s"), t->msg_sender);
		tmp = g_strconcat(msg, "\n", str, NULL);

		g_free(msg);
		g_free(str);
		msg = tmp;
	}
	if (t->msg_subject) {
		gchar *tmp, *str;

		/* Translators: "Subject:" is preceding a new mail
		 * subject, like "Subject: It happened again"
		 */
		str = g_strdup_printf(_("Subject: %s"), t->msg_subject);
		tmp = g_strconcat(msg, "\n", str, NULL);

		g_free(msg);
		g_free(str);
		msg = tmp;
	}

	set_icon(_onicon, TRUE, msg);

	_onicon->winnotify = TRUE;

	g_free(msg);
}
/* Start up and put our icon */
void
org_gnome_evolution_tray_startup(void *ep)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	if (!on_icon.quit_func)
		create_icon(&on_icon, do_properties, do_quit, toggle_window);
}
/* Nofity based on folder changes */
void
org_gnome_evolution_on_folder_changed(EPlugin *ep, EMEventTargetFolder *t)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	/* TODO:
	 * try to update state according what is changed in the folder. Note -
	 * getting the folder may block...
	 */
	if (t->new > 0)
		new_notify_status(t, &on_icon);
}
/* Mail read nofity */
void
org_gnome_mail_read_notify(EPlugin *ep, EMEventTargetMessage *t)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	if (g_atomic_int_compare_and_exchange(&on_icon.status_count, 0, 0))
		return;

	CamelMessageInfo *info = camel_folder_get_message_info(t->folder, t->uid);
	if (info) {
		guint flags = camel_message_info_get_flags(info);
		if (!(flags & CAMEL_MESSAGE_SEEN)) {
			if (g_atomic_int_dec_and_test(&on_icon.status_count))
				set_icon(&on_icon, FALSE, _(""));
		}
#if EVOLUTION_VERSION < 31192
		camel_folder_free_message_info(t->folder, info);
#else
		g_clear_object(&info);
#endif
	}
}
/* Change window state */
static gboolean
window_state_event(GtkWidget *widget, GdkEventWindowState *event)
{
	gint x, y; /* to save window position */
	gint width, height; /* to save window size */
	
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	if (is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDE_ON_MINIMIZE)
			&& (event->changed_mask == GDK_WINDOW_STATE_ICONIFIED)) {
		/* GTK documentation says that it is not rediable way to save 
		 * and restore window postion and we should use the native windowing
		 * APIs instead. However, I am not digging into xlib yet.*/
		 gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
		 gtk_window_get_size(GTK_WINDOW(widget), &width, &height);
#ifdef DEBUG
		g_printf("Evolution-on: Window Positions: x:%i y:%i\n", x, y);
		g_printf("Evolution-on: Window Sizes: width:%i height:%i\n", width, height);
#endif
		gtk_window_set_default_size(GTK_WINDOW(widget), width, height);
		if (event->new_window_state && GDK_WINDOW_STATE_ICONIFIED) {
			on_icon.toggle_window_func();
		} else {
			gtk_window_deiconify(GTK_WINDOW(widget));
			gtk_window_move(GTK_WINDOW(widget), x, y);
		}
	}
	return FALSE;
}
/* Handle window deletion */
gboolean
on_widget_deleted(GtkWidget *widget, GdkEvent * /*event*/, gpointer /*data*/)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	if(is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDE_ON_CLOSE)) {
		on_icon.toggle_window_func();
		return TRUE; // we've handled it
	}
	return FALSE;
}

gboolean
e_plugin_ui_init(EUIManager *ui_manager, EShellView *shell_view)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	GdkDisplay *display;
	GdkMonitor *monitor;
	GdkRectangle geometry;
	
	display = gdk_display_get_default();
	monitor = gdk_display_get_monitor(display, 0);
	gdk_monitor_get_geometry(monitor, &geometry);
	
	on_icon.evo_window = e_shell_view_get_shell_window(shell_view);

	show_window_handle = g_signal_connect(G_OBJECT(on_icon.evo_window),
			"show", G_CALLBACK(shown_window_cb), &on_icon);

	g_signal_connect(G_OBJECT(on_icon.evo_window), "window-state-event",
			G_CALLBACK(window_state_event), NULL);

	g_signal_connect(G_OBJECT(on_icon.evo_window), "delete-event",
            G_CALLBACK(on_widget_deleted), NULL);

	if (!on_icon.quit_func)
		create_icon(&on_icon, do_properties, do_quit, toggle_window);

	return TRUE;
}

GtkWidget *
e_plugin_lib_get_configure_widget(EPlugin *epl)
{
	return get_cfg_widget();
}
