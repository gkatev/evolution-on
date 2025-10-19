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

#include "sn.h"
#include "ucount.h"
#include "properties.h"

#define ICON_READ "mail-read"
#define ICON_UNREAD "mail-unread"

static EShellWindow *shell_window = NULL;

static gboolean initialized = FALSE;
static gboolean hide_startup = FALSE;

static enum {
	STATUS_READ,
	STATUS_UNREAD
} status = STATUS_READ;

// -----------------------------

static void hide_window(void) {
	gtk_widget_hide(GTK_WIDGET(shell_window));
}

static void show_window(void) {
	gtk_widget_show(GTK_WIDGET(shell_window));
}

static void set_read(gboolean set_checkpoint) {
	if(status == STATUS_UNREAD) {
		sn_set_icon(ICON_READ);
		status = STATUS_READ;
		
		/* We are now in the 'read' status. The user now knows about
		 * all new emails. Set this as our new known status. We'll only
		 * notify the user about new email relative to this new status.
		 * See also comments in ucount.c. */
		if(set_checkpoint)
			ucount_set_checkpoint();
	}
}

static void set_unread(void) {
	if(status == STATUS_READ) {
		sn_set_icon(ICON_UNREAD);
		status = STATUS_UNREAD;
	}
}

/* Called when all folders revert back to the same unread mail
 * count as the last time that the application was opened/focused. */
static void on_ucount_checkpoint(void) {
	set_read(FALSE);
}

static void switch_mail_view(void) {
	e_shell_window_set_active_view(shell_window, "mail");
}

static gboolean in_mail_view(void) {
	return g_str_equal(e_shell_window_get_active_view(shell_window), "mail");
}

static void on_activate(void) {
	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(shell_window));
	GdkWindowState window_state = gdk_window_get_state(gdk_window);
	
	/* If the window is iconfied, we want it to
	 * come up when we click on the tray icon. */
	if(window_state & GDK_WINDOW_STATE_ICONIFIED) {
		gtk_window_deiconify(GTK_WINDOW(shell_window));
		return;
	}
	
	gboolean unread = (status == STATUS_UNREAD);
	
	if(gtk_widget_get_visible(GTK_WIDGET(shell_window))) {
		/* The window is visible, the icon indicates new mail, and the user
		 * clicked on it. Would be weird to hide it, no? (Try to) bring it
		 * to it to the foreground instead. */
		if(unread) {
			gtk_window_present(GTK_WINDOW(shell_window));
			switch_mail_view();
			set_read(TRUE);
		} else
			hide_window();
	} else {
		show_window();
		
		/* The window was hidden, the icon indicates new mail,
		 * and the user clicked on it -> focus the mail view. */
		if(unread)
			switch_mail_view();
	}
}

static void do_properties(void) {
	properties_show();
}

static void do_quit(void) {
	EShell *shell = e_shell_get_default();
	e_shell_quit(shell, E_SHELL_QUIT_ACTION);
}

// -----------------------------

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

static gboolean on_window_state_event(GtkWidget *widget,
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

static void on_window_show(GtkWidget *widget, gpointer user_data) {
	/* If enabled, the first time the evolution
	 * window is shown, hide it to the tray. */
	if(hide_startup) {
		hide_window();
		hide_startup = FALSE;
	}
	
	if(in_mail_view())
		set_read(TRUE);
}

static void on_window_focus_in(GtkWidget *widget,
	GdkEventFocus *event, gpointer user_data)
{
	if(in_mail_view())
		set_read(TRUE);
}

static void on_active_view_change(EShellWindow * /* shell_window */) {
	if(in_mail_view())
		set_read(TRUE);
}

// -----------------------------

void org_gnome_mail_folder_unread_updated(EPlugin *ep,
	EMEventTargetFolderUnread *t)
{
	// Apparently, this can happen.
	if(t->unread == (guint) -1)
		return;
	
	// Update our internal per-folder unread count record
	gint delta = ucount_event(t->folder_uri, t->unread);
	
	if(delta > 0)
		set_unread();
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
	gint err;
	
	// When called from e_plugin_lib_enable
	if(!shell_window) {
		if(!(shell_window = find_shell_window()))
			return -1;
	}
	
	err = sn_init(ICON_READ, on_activate, do_properties, do_quit);
	if(err != 0) {
		g_printerr("Evolution-on: StatusNotifierItem init failed (%d)\n", err);
		return -2;
	}
	
	err = ucount_init(on_ucount_checkpoint);
	if(err != 0) {
		sn_fini();
		g_printerr("Evolution-on: Ucount init failed (%d)\n", err);
		return -3;
	}
	
	g_signal_connect(G_OBJECT(shell_window), "show",
		G_CALLBACK(on_window_show), NULL);
	
	g_signal_connect(G_OBJECT(shell_window), "focus-in-event",
		G_CALLBACK(on_window_focus_in), NULL);
	
	g_signal_connect(G_OBJECT(shell_window), "window-state-event",
			G_CALLBACK(on_window_state_event), NULL);
	
	g_signal_connect(G_OBJECT(shell_window), "delete-event",
		G_CALLBACK(on_widget_deleted), NULL);
	
	g_signal_connect(G_OBJECT(shell_window), "notify::active-view",
		G_CALLBACK(on_active_view_change), NULL);
	
	status = STATUS_READ;
	initialized = TRUE;
	
	return 0;
}

static void fini(void) {
	g_signal_handlers_disconnect_by_func(shell_window, on_window_show, NULL);
	g_signal_handlers_disconnect_by_func(shell_window, on_window_focus_in, NULL);
	g_signal_handlers_disconnect_by_func(shell_window, on_window_state_event, NULL);
	g_signal_handlers_disconnect_by_func(shell_window, on_widget_deleted, NULL);
	
	g_signal_handlers_disconnect_by_func(shell_window, on_active_view_change, NULL);
	
	ucount_fini();
	sn_fini();
	
	show_window();
	
	shell_window = NULL;
	initialized = FALSE;
	status = STATUS_READ;
}

gboolean e_plugin_ui_init(EUIManager *ui_manager, EShellView *shell_view) {
	/* If hide-on-startup is enabled, mark the pending hide-action.
	 * We only do this from ui_init(), i.e. only when evolution is
	 * actually starting up, not if our plugin is merely being
	 * enabled at a later point. */
	if(is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDDEN_ON_STARTUP))
		hide_startup = TRUE;
	
	gint err = 0;
	
	if(!initialized) {
		shell_window = e_shell_view_get_shell_window(shell_view);
		err = init();
	}
	
	return (err == 0);
}

gint e_plugin_lib_enable(EPlugin *ep, gint enable) {
	if(enable && !initialized)
		return init();
	else if(!enable && initialized)
		fini();
	
	return 0;
}
