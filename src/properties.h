#ifndef EVOLUTION_ON_ON_PROPERTIES_H
#define EVOLUTION_ON_ON_PROPERTIES_H

#define GCONF_KEY_NOTIF_ROOT			"/apps/evolution/eplugin/mail-notification/"
#define GCONF_KEY_TRAY_ROOT				"/apps/evolution/eplugin/evolution-on/"

#define NOTIF_SCHEMA					"org.gnome.evolution.plugin.mail-notification"
#define TRAY_SCHEMA						"org.gnome.evolution.plugin.evolution-on"

#define CONF_KEY_HIDDEN_ON_STARTUP		"hidden-on-startup"
#define CONF_KEY_HIDE_ON_MINIMIZE		"hide-on-minimize"
#define CONF_KEY_HIDE_ON_CLOSE			"hide-on-close"
#define CONF_KEY_NOTIFY_ONLY_INBOX		"notify-only-inbox"
#define CONF_KEY_ENABLED_DBUS			"notify-dbus-enabled"
#define CONF_KEY_ENABLED_STATUS			"notify-status-enabled"
#define CONF_KEY_ENABLED_SOUND			"notify-sound-enabled"
#define CONF_KEY_STATUS_NOTIFICATION	"notify-status-notification"

gboolean is_part_enabled(gchar *schema, const gchar *key);
void properties_show(void);

#endif /* EVOLUTION_ON_ON_PROPERTIES_H */
