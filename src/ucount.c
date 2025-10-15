/* Evoution On plugin
 *  Copyright (C) 2025 George Katevenis <george_kate@hotmail.com>
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

#include <gio/gio.h>
#include <glib/gprintf.h>

#include "ucount.h"

typedef struct unode_t {
	guint count;
	guint checkpoint;
} unode_t;

static GHashTable *utable = NULL;
static void (*global_checkpoint_reached_cb)(void) = NULL;

static gint n_folders_over_checkpoint = 0;

gint ucount_init(void (*checkpoint_cb)(void)) {
	printf("init\n");
	
	utable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	if(!utable) return -1;
	
	global_checkpoint_reached_cb = checkpoint_cb;
	
	return 0;
}

void ucount_fini(void) {
	g_clear_pointer(&utable, g_hash_table_destroy);
	n_folders_over_checkpoint = 0;
	global_checkpoint_reached_cb = NULL;
}

static void ucount_insert(const gchar *folder, guint count) {
	unode_t *unode = g_malloc(sizeof(unode_t));
	gchar *key = g_strdup(folder);
	
	if(!unode || !key) {
		g_free(unode);
		g_free(key);
		return;
	}
	
	*unode = (unode_t) {.count = count, .checkpoint = count};
	g_hash_table_insert(utable, key, unode);
	
	printf("insert %s:{%u, %u}\n", key, count, count);
}

gint ucount_event(const gchar *folder, guint count) {
	unode_t *unode = g_hash_table_lookup(utable, folder);
	
	if(!unode) {
		ucount_insert(folder, count);
		return 0;
	}
	
	guint prev_count = unode->count;
	gboolean was_at_checkpoint = (prev_count == unode->checkpoint);
	
	unode->count = count;
	
	printf("%s: %u -> %u (check: %u)\n",
		folder, prev_count, count, unode->checkpoint);
	
	if(count > prev_count) {
		if(was_at_checkpoint) {
			n_folders_over_checkpoint++;
			printf("no longer at checkpoint, now have %d folders over checkpoint\n",
				n_folders_over_checkpoint);
		}
	} else if(count < prev_count) {
		if(count <= unode->checkpoint) {
			
			unode->checkpoint = count;
			
			if(count < unode->checkpoint)
				printf("checkpoint now lower: %d\n",
					unode->checkpoint);
			
			if(!was_at_checkpoint) {
				n_folders_over_checkpoint--;
				
				printf("now at checkpoint, have %d folder over checkpoint\n",
					n_folders_over_checkpoint);
				
				if(n_folders_over_checkpoint == 0) {
					printf("all folders at checkpoint, call cb\n");
					global_checkpoint_reached_cb();
				}
			}
		}
	}
	
	g_assert(n_folders_over_checkpoint >= 0);
	
	return count - prev_count;
}

/* void ucount_event_dud(const gchar *folder, guint count) {
	unode_t *unode = g_hash_table_lookup(utable, folder);
	
	if(!unode) {
		ucount_insert(folder, count);
		return;
	}
	
	gboolean was_at_checkpoint = (unode->count == unode->checkpoint);
	
	printf("%s: %u -> %u (check: %u)\n",
		folder, unode->count, count, unode->checkpoint);
	
	if(!was_at_checkpoint) {
		n_folders_over_checkpoint--;
		
		printf("we weren't in checkpoint, now we are: %d\n", count);
		
		if(n_folders_over_checkpoint == 0) {
			printf("all folders at checkpoint, call cb\n");
			global_checkpoint_reached_cb();
		}
	}
	
	if(count != unode->checkpoint)
		printf("checkpoint now at: %d\n",
			count);
	else
		printf("was already at checkpoint (%d)\n",
			count);
	
	unode->count = count;
	unode->checkpoint = count;
} */

static void update_checkpoint_foreach_cb(gpointer key, gpointer value,
	gpointer user_data)
{
	unode_t *unode = (unode_t *) value;
	if(unode->checkpoint != unode->count)
		printf("%s was at checkpoint %d, now at %d\n",
			(char *) key, unode->checkpoint, unode->count);
	unode->checkpoint = unode->count;
}

void ucount_set_checkpoint(void) {
	printf("update_checkpoint\n");
	
	g_hash_table_foreach(utable, update_checkpoint_foreach_cb, NULL);
	n_folders_over_checkpoint = 0;
}
