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

#include <string.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include <e-util/e-util.h>

#include <shell/e-shell.h>
#include <shell/e-shell-view.h>
#include <shell/e-shell-window.h>

#include <mail/em-event.h>
#include <mail/em-folder-tree.h>
#include <mail/e-mail-reader.h>

#include <libemail-engine/libemail-engine.h>

#include "sn.h"
#include "properties.h"

#define ICON_READ "mail-read"
#define ICON_UNREAD "mail-unread"

static EShellWindow *evo_window;
static gboolean initialized = FALSE;
static gboolean hide_startup = FALSE;

// -----------------------------

static void hide_window(void) {
	gtk_widget_hide(GTK_WIDGET(evo_window));
}

static void show_window(void) {
	gtk_widget_show(GTK_WIDGET(evo_window));
}

static void toggle_window(void) {
	if(gtk_widget_get_visible(GTK_WIDGET(evo_window)))
		hide_window();
	else
		show_window();
}

static void do_properties(void) {
	properties_show();
}

static void do_quit(void) {
	EShell *shell = e_shell_get_default();
	e_shell_quit(shell, E_SHELL_QUIT_ACTION);
}

// -----------------------------

static void shown_window_cb(GtkWidget *widget, gpointer user_data) {
	/* If enabled, the first time the evolution
	 * window is shown, hide it to the tray. */
	
	if(hide_startup) {
		hide_window();
		hide_startup = FALSE;
	}
	
	sn_set_icon(ICON_READ);
}

static gboolean window_state_event(GtkWidget *widget,
	GdkEventWindowState *event)
{
	/* If enabled, when minimizing, hide to tray instead.
	 *
	 * Hide the window, then call deiconify, so that the next time we show
	 * it, it won't be minimized. IINM, de-iconifying a hidden window does
	 * not make it appear at once, but updates its internal state accordingly.
	 *
	 * These actions themselves will trigger a bunch of window state events,
	 * and we don't really want to act on them. For sure, as soon as hide(),
	 * all subsequently emitted events will have the WITHDRAWN flag, so just
	 * ignore all invocations that contain it. */
	
	if(is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDE_ON_MINIMIZE)
		&& (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED)
		&& (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)
		&& !(event->new_window_state & GDK_WINDOW_STATE_WITHDRAWN))
	{
		hide_window();
		gtk_window_deiconify(GTK_WINDOW(widget));
	}
	
	return FALSE;
}

static gboolean on_widget_deleted(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
{
	/* If enabled, abort the window-close and hide it instead. */
	
	if(is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDE_ON_CLOSE)) {
		hide_window();
		return TRUE; // we've handled it, don't run any more handlers
	}
	
	return FALSE;
}

// -----------------------------

static EShellWindow *find_shell_window(void) {
	EShell *shell = e_shell_get_default();
	if(!shell) return NULL;
	
	for(GList *list = gtk_application_get_windows(GTK_APPLICATION(shell));
		list != NULL; list = g_list_next(list))
	{
		if(E_IS_SHELL_WINDOW(list->data))
			return E_SHELL_WINDOW(list->data);
	}
	
	g_printerr("Evolution-on: Couldn't get EShell Window\n");
	return NULL;
}

static gint init(void) {
	if(!evo_window && !(evo_window = find_shell_window()))
		return -1;
	
	int status = sn_init(ICON_READ, toggle_window, do_properties, do_quit);
	if(status != 0) {
		g_printerr("Evolution-on: StatusNotifierItem init failed (%d)\n", status);
		return -2;
	}
	
	g_signal_connect(G_OBJECT(evo_window), "show",
		G_CALLBACK(shown_window_cb), NULL);
	
	g_signal_connect(G_OBJECT(evo_window), "window-state-event",
			G_CALLBACK(window_state_event), NULL);
	
	g_signal_connect(G_OBJECT(evo_window), "delete-event",
		G_CALLBACK(on_widget_deleted), NULL);
	
	initialized = TRUE;
	return 0;
}

static void fini(void) {
	g_signal_handlers_disconnect_by_func(evo_window, shown_window_cb, NULL);
	g_signal_handlers_disconnect_by_func(evo_window, window_state_event, NULL);
	g_signal_handlers_disconnect_by_func(evo_window, on_widget_deleted, NULL);
	
	show_window();
	
	sn_fini();
	
	initialized = FALSE;
}

// -----------------------------

void org_gnome_evolution_on_folder_changed(EPlugin *ep, EMEventTargetFolder *t) {
	// printf("org_gnome_evolution_on_folder_changed\n");
	
	/* TODO:
	 * try to update state according what is changed in the folder. Note -
	 * getting the folder may block...
	 */
	if (t->new > 0) {
		sn_set_icon(ICON_UNREAD);
		
		// sn_set_tooltip(msg);
		// printf("tooltip: %s\n", msg);
	}
}

void org_gnome_mail_read_notify(EPlugin *ep, EMEventTargetMessage *t) {
	printf("org_gnome_mail_read_notify\n");
	
	// if (g_atomic_int_compare_and_exchange(&on_icon.status_count, 0, 0))
		// return;

	// CamelMessageInfo *info = camel_folder_get_message_info(t->folder, t->uid);
	// if (info) {
		// guint flags = camel_message_info_get_flags(info);
		// if (!(flags & CAMEL_MESSAGE_SEEN)) {
			// if (g_atomic_int_dec_and_test(&on_icon.status_count))
				// sn_set_icon(ICON_READ);
		// }
// #if EVOLUTION_VERSION < 31192
		// camel_folder_free_message_info(t->folder, info);
// #else
		// g_clear_object(&info);
// #endif
	// }
}

gboolean e_plugin_ui_init(EUIManager *ui_manager, EShellView *shell_view) {
	/* If hide-on-startup is enabled, mark the pending hide-action.
	 * We only do this from ui_init(), i.e. only when evolution is
	 * actually starting up, not if our plugin is merely being
	 * enabled at a later point. */
	if(is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDDEN_ON_STARTUP))
		hide_startup = TRUE;
	
	gint status = 0;
	
	if(!initialized) {
		evo_window = e_shell_view_get_shell_window(shell_view);
		status = init();
	}
	
	return (status == 0);
}

gint e_plugin_lib_enable(EPlugin *ep, gint enable) {
	if(enable && !initialized)
		return init();
	else if(!enable && initialized)
		fini();
	
	return 0;
}
