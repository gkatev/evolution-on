/* Evoution On plugin
 *  Copyright (C) 2008-2012 Lucian Langa <cooly@gnome.eu.org>
 *  Copyright (C) 2022 Ozan Türkyılmaz <ozan.turkyilmaz@gmail.com>
 *  Copyright (C) 2023-2025 George Katevenis <george_kate@hotmail.com>
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

#include "on_icon.h"
#include "properties.h"
#include "sn.h"

#define ICON_READ "mail-read"
#define ICON_UNREAD "mail-unread"

// static gulong show_window_handle = 0;
// static gboolean show_window_cb_called = FALSE;
static gboolean initialized = FALSE;
struct OnIcon on_icon = ONICON_NEW;

//helper method for toggling used on init for hidden on startup and on tray click
static void
toggle_window(void)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	if (gtk_widget_get_visible(GTK_WIDGET(on_icon.evo_window))) {
		gtk_widget_hide(GTK_WIDGET(on_icon.evo_window));
	} else {
		gtk_widget_show(GTK_WIDGET(on_icon.evo_window));
		sn_set_icon(ICON_READ);
	}

	// if (on_icon.winnotify) {
		// set_icon(&on_icon, FALSE, _(""));
		// on_icon.winnotify = FALSE;
	// }
}

static void
do_quit(void)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	EShell *shell = e_shell_get_default();
	e_shell_quit(shell, E_SHELL_QUIT_ACTION);
}

static void
do_properties(void)
{
	properties_show();
}

/* Show window when clicked on our icon */
static void
shown_window_cb(GtkWidget *widget, gpointer user_data)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	
	static gboolean show_window_cb_called = FALSE;
	
	if (!show_window_cb_called) {
		if (is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDDEN_ON_STARTUP)) {
			// on_icon.toggle_window_func();
			toggle_window();
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

	sn_set_icon(ICON_UNREAD);
	// sn_set_tooltip(msg);
	
	// printf("tooltip: %s\n", msg);

	// _onicon->winnotify = TRUE;

	g_free(msg);
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
				// set_icon(&on_icon, FALSE, _(""));
				sn_set_icon(ICON_READ);
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
			toggle_window();
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
		toggle_window();
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

	g_signal_connect(G_OBJECT(on_icon.evo_window),
		"show", G_CALLBACK(shown_window_cb), NULL);

	g_signal_connect(G_OBJECT(on_icon.evo_window), "window-state-event",
			G_CALLBACK(window_state_event), NULL);

	g_signal_connect(G_OBJECT(on_icon.evo_window),
		"delete-event", G_CALLBACK(on_widget_deleted), NULL);

	if(!initialized)
		sn_init(ICON_READ, toggle_window, do_properties, do_quit);

	on_icon.properties_func = do_properties;
	on_icon.quit_func = do_quit;
	on_icon.toggle_window_func = toggle_window;
	// on_icon.winnotify = FALSE;

	return TRUE;
}

void // TODO Is this useful?
org_gnome_evolution_tray_startup(void *ep)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	
	if(!initialized)
		sn_init(ICON_READ, toggle_window, do_properties, do_quit);
}
