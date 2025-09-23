/* Evoution On plugin
 * Copyright (C) 2008-2012 Lucian Langa <cooly@gnome.eu.org>
 * Copyright (C) 2012-2013 Kostadin Atanasov <pranayama111@gmail.com>
 * Copyright (C) 2022 Ozan Türkyılmaz <ozan.turkyilmaz@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef EVOLUTION_ON_ON_ICON_H
#define EVOLUTION_ON_ON_ICON_H

#ifdef DEBUG
#include <glib/gprintf.h>
#endif

typedef void (*do_properties_func)(GtkMenuItem*, gpointer);
typedef void (*do_quit_func)(GtkMenuItem*, gpointer);
typedef void (*do_toggle_window_func)();

struct OnIcon {
	GtkStatusIcon			*icon;

	EShellWindow			*evo_window;

	do_properties_func		properties_func;
	do_quit_func			quit_func;
	do_toggle_window_func	toggle_window_func;

	gchar					*uri;
	guint					status_count;
	gboolean				winnotify;
}; /* struct OnIcon */

#define ONICON_NEW {NULL}

static void
remove_notification(struct OnIcon *_onicon);
static void
status_icon_activate_cb(struct OnIcon *_onicon);

static gboolean
button_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer data);

static void
icon_activated(GtkStatusIcon *icon, gpointer user_data);

static void
popup_menu_status(GtkStatusIcon *status_icon, guint button,
		guint activate_time, gpointer user_data);

static GtkMenu *
create_popup_menu(struct OnIcon *_onicon);

static void
set_icon(struct OnIcon *_onicon, gboolean unread, const gchar *msg)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	gtk_status_icon_set_tooltip_text(_onicon->icon, msg);
	if (unread) {
		gtk_status_icon_set_from_pixbuf(_onicon->icon,
				e_icon_factory_get_icon("mail-unread",
						GTK_ICON_SIZE_DIALOG));
	} else {
		gtk_status_icon_set_from_pixbuf(_onicon->icon,
				e_icon_factory_get_icon("mail-read",
						GTK_ICON_SIZE_DIALOG));
	}
}

static void
create_icon(struct OnIcon *_onicon,
		do_properties_func _prop_func,
		do_quit_func _quit_func,
		do_toggle_window_func _toggle_window_func)
{
	_onicon->properties_func = _prop_func;
	_onicon->quit_func = _quit_func;
	_onicon->toggle_window_func = _toggle_window_func;
	_onicon->winnotify = FALSE;

	if (!_onicon->icon) {
		_onicon->icon = gtk_status_icon_new();
		gtk_status_icon_set_from_pixbuf(_onicon->icon,
				e_icon_factory_get_icon("mail-read",
						GTK_ICON_SIZE_DIALOG));

		g_signal_connect(G_OBJECT(_onicon->icon), "activate",
				G_CALLBACK(icon_activated),
				_onicon);

		g_signal_connect(G_OBJECT(_onicon->icon),"button-press-event",
				G_CALLBACK(button_press_cb), _onicon);

		g_signal_connect(_onicon->icon, "popup-menu",
				G_CALLBACK(popup_menu_status), _onicon);
	}
	gtk_status_icon_set_visible(_onicon->icon, TRUE);

}

static void
icon_activated(GtkStatusIcon *icon, gpointer user_data)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	struct OnIcon *_onicon = (struct OnIcon*)user_data;
	status_icon_activate_cb(_onicon);
	gtk_status_icon_set_from_pixbuf (_onicon->icon,
			e_icon_factory_get_icon("mail-read", GTK_ICON_SIZE_DIALOG));
	gtk_status_icon_set_has_tooltip (_onicon->icon, FALSE);
	_onicon->winnotify = FALSE;
}

static void
popup_menu_status(GtkStatusIcon *status_icon, guint button,
		guint activate_time, gpointer user_data)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	struct OnIcon *_onicon = (struct OnIcon*)user_data;
	GtkMenu *menu = create_popup_menu(_onicon);
	gtk_menu_popup(GTK_MENU(menu),
			NULL, NULL,
			gtk_status_icon_position_menu,
			_onicon->icon,
			button, activate_time);
}

static gboolean
button_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	struct OnIcon *_onicon = (struct OnIcon*)data;
	if (event->button != 1) {
		return FALSE;
	}
	_onicon->toggle_window_func();
	return TRUE;
}

static GtkMenu *
create_popup_menu(struct OnIcon *_onicon)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	GtkMenu *menu;
	GtkWidget *item;

	menu = GTK_MENU(gtk_menu_new());

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
	g_signal_connect(item, "activate",
			G_CALLBACK(_onicon->properties_func), _onicon);

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
	g_signal_connect(item, "activate",
			G_CALLBACK(_onicon->quit_func), _onicon);
	return menu;
}

static void
status_icon_activate_cb(struct OnIcon *_onicon)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	EShell *shell;
	GList *list;
	GtkApplication *application;

	shell = e_shell_get_default();
	application = GTK_APPLICATION(shell);
	list = gtk_application_get_windows(application);

	/* Find the first EShellWindow in the list. */
	while (list != NULL && !E_IS_SHELL_WINDOW(list->data))
		list = g_list_next(list);

	g_return_if_fail(list != NULL);

	if (_onicon->uri) {
		EShellView *shell_view;
		EShellWindow *shell_window;
		EShellSidebar *shell_sidebar;
		EMFolderTree *folder_tree;
		EUIAction *action;
		/* Present the shell window. */
		shell_window = E_SHELL_WINDOW(list->data);

		/* Switch to the mail view. */
		shell_view = e_shell_window_get_shell_view(shell_window, "mail");
		action = e_shell_view_get_switcher_action(shell_view);
		e_ui_action_set_active(action, TRUE);

		/* Select the latest folder with new mail. */
		shell_sidebar = e_shell_view_get_shell_sidebar(shell_view);
		g_object_get(shell_sidebar, "folder-tree", &folder_tree, NULL);
		em_folder_tree_set_selected(folder_tree, _onicon->uri, FALSE);

		_onicon->uri = NULL;
	}

	remove_notification(_onicon);
}

static void
remove_notification(struct OnIcon *_onicon)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	_onicon->status_count = 0;
}

#endif /* EVOLUTION_ON_ON_ICON_H */
