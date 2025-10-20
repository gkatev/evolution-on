#ifndef EVOLUTION_TRAY_PROPERTIES_H
#define EVOLUTION_TRAY_PROPERTIES_H

#define GCONF_KEY_TRAY_ROOT				"/apps/evolution/eplugin/evolution-tray/"
#define TRAY_SCHEMA						"org.gnome.evolution.plugin.evolution-tray"

#define CONF_KEY_HIDDEN_ON_STARTUP		"hidden-on-startup"
#define CONF_KEY_HIDE_ON_MINIMIZE		"hide-on-minimize"
#define CONF_KEY_HIDE_ON_CLOSE			"hide-on-close"

gboolean is_part_enabled(gchar *schema, const gchar *key);
void properties_show(void);

#endif /* EVOLUTION_TRAY_PROPERTIES_H */
