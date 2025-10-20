#ifndef EVOLUTION_TRAY_UCOUNT_H
#define EVOLUTION_TRAY_UCOUNT_H

gint ucount_init(void (*checkpoint_cb)(void));
void ucount_fini(void);

gint ucount_event(const gchar *folder, guint count);
// void ucount_event_dud(const gchar *folder, guint count);
void ucount_set_checkpoint(void);

#endif
