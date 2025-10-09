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

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#ifdef DEBUG
#include <glib/gprintf.h>
#endif

#include <e-util/e-util.h>

#include "properties.h"

/******************************************************************************
 * Query dconf
 *****************************************************************************/
gboolean
is_part_enabled(gchar *schema, const gchar *key)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	GSettings *settings = g_settings_new(schema);
	gboolean res = g_settings_get_boolean(settings, key);
	g_object_unref(settings);
	return res;
}

static void
set_part_enabled(gchar *schema, const gchar *key, gboolean enable)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	GSettings *settings = g_settings_new (schema);
	g_settings_set_boolean (settings, key, enable);
	g_object_unref (settings);
}

/******************************************************************************
 * Callback for configuration widget
 *****************************************************************************/
static void
toggled_hidden_on_startup_cb(GtkWidget *widget, gpointer data)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	g_return_if_fail(widget != NULL);
	set_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDDEN_ON_STARTUP,
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

static void
toggled_hidde_on_minimize_cb(GtkWidget *widget, gpointer data)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	g_return_if_fail(widget != NULL);
	set_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDE_ON_MINIMIZE,
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

static void
toggle_hidden_on_close_cb(GtkWidget *widget, gpointer data)
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	g_return_if_fail(widget != NULL);
	set_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDE_ON_CLOSE,
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/******************************************************************************
 * Properties widget
 *****************************************************************************/

static GtkWidget *
get_cfg_widget()
{
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	GtkWidget *container, *vbox, *check;
	vbox = gtk_box_new (FALSE, 6);
	gtk_widget_show (vbox);
	container = vbox;

	check = gtk_check_button_new_with_mnemonic(_("Hidden on startup"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDDEN_ON_STARTUP));
	g_signal_connect(G_OBJECT(check), "toggled",
			G_CALLBACK(toggled_hidden_on_startup_cb), NULL);
	gtk_widget_show(check);
	gtk_box_pack_start(GTK_BOX(container), check, FALSE, FALSE, 0);

	check = gtk_check_button_new_with_mnemonic(_("Hide on minimize"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (check),
			is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDE_ON_MINIMIZE));
	g_signal_connect(G_OBJECT (check), "toggled",
			G_CALLBACK(toggled_hidde_on_minimize_cb), NULL);
	gtk_widget_show(check);
	gtk_box_pack_start(GTK_BOX(container), check, FALSE, FALSE, 0);

	check = gtk_check_button_new_with_mnemonic(_("Hide on close"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			is_part_enabled(TRAY_SCHEMA, CONF_KEY_HIDE_ON_CLOSE));
	g_signal_connect(G_OBJECT(check), "toggled",
			G_CALLBACK(toggle_hidden_on_close_cb), NULL);
	gtk_widget_show(check);
	gtk_box_pack_start(GTK_BOX(container), check, FALSE, FALSE, 0);

	return container;
}

GtkWidget *
e_plugin_lib_get_configure_widget(EPlugin *epl)
{
	return get_cfg_widget();
}

void properties_show(void) {
#ifdef DEBUG
	g_printf("Evolution-on: Function call %s\n", __func__);
#endif
	GtkWidget *cfg, *dialog, *label, *vbox, *hbox;
	GtkWidget *content_area;
	gchar *text;

	cfg = get_cfg_widget();
	if (!cfg)
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

	dialog = gtk_dialog_new_with_buttons(_("Mail Notification Properties"),
			NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			_("_Close"), GTK_RESPONSE_CLOSE, NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	gtk_container_add(GTK_CONTAINER(content_area), vbox);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), 10);
	gtk_widget_set_size_request(dialog, 400, -1);
	g_signal_connect_swapped(dialog, "response",
			G_CALLBACK(gtk_widget_destroy), dialog);
	gtk_widget_show(dialog);	
}
