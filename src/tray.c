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

static
void hide_window(void)
{
	gtk_widget_hide(GTK_WIDGET(evo_window));
}

static
void show_window(void)
{
	gtk_widget_show(GTK_WIDGET(evo_window));
}

static void
toggle_window(void)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	if(gtk_widget_get_visible(GTK_WIDGET(evo_window))) {
		hide_window();
	} else {
		show_window();
	}
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
			toggle_window();
		}
		show_window_cb_called = TRUE;
	}
	
	sn_set_icon(ICON_READ);
}

/* New email notification */
static void
new_notify_status(EMEventTargetFolder *t)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	
	sn_set_icon(ICON_UNREAD);

	// sn_set_tooltip(msg);
	// printf("tooltip: %s\n", msg);
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
		new_notify_status(t);
}
/* Mail read nofity */
void
org_gnome_mail_read_notify(EPlugin *ep, EMEventTargetMessage *t)
{
// #ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
// #endif
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

static gboolean
window_state_event(GtkWidget *widget, GdkEventWindowState *event)
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
/* Handle window deletion */
gboolean
on_widget_deleted(GtkWidget *widget, GdkEvent * /*event*/, gpointer /*data*/)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	if(is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDE_ON_CLOSE)) {
		hide_window();
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

	evo_window = e_shell_view_get_shell_window(shell_view);

	g_signal_connect(G_OBJECT(evo_window),
		"show", G_CALLBACK(shown_window_cb), NULL);

	g_signal_connect(G_OBJECT(evo_window), "window-state-event",
			G_CALLBACK(window_state_event), NULL);

	g_signal_connect(G_OBJECT(evo_window),
		"delete-event", G_CALLBACK(on_widget_deleted), NULL);

	if(!initialized)
		sn_init(ICON_READ, toggle_window, do_properties, do_quit);

	return TRUE;
}

void // TODO Is this useful?
org_gnome_evolution_tray_startup(void *ep)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	
	printf("org_gnome_evolution_tray_startup\n");
	
	if(!initialized)
		sn_init(ICON_READ, toggle_window, do_properties, do_quit);
}
