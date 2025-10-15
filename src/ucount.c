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

/* The ucount (i.e. 'unread count', yes I know, naming things is hard...)
 * serves to keep track of the number of unread mails in each folder. This
 * is essentially necessary because Evolution does not really give us an
 * aggregate unread mail count for all folder. Imagine you get an event
 * "folder X has 4 unread mails" -- okay, is this new? How many did it
 * have before? Hence this folder unread count table!
 *
 * We implement this memory structure using a hash table, keyed with the
 * folder URIs. We mainly track the count of unread mails in the folder.
 * When we receive an unread count event, we can compare with the entry
 * in the table to determine if there's a new email.
 *
 * We also want to know when emails have been read, so that we may undo
 * the 'unread' status, in scenarios where all new mails were read in
 * another client. This is also be achieved with the count entry (but
 * see below for caveat). To avoid checking the entire hash table
 * whenever we get a new event, we keep a table-global counter that
 * tracks how many folders have new emails at any given time.
 *
 * But there's a twist: what if the user has a bunch of emails they leave
 * on unread? In such a scenario, the icon might get perpetually stuck in
 * 'unread' status. To attempt to tackle this, we also use a secondary
 * variable: checkpoint. This is simply the number of unread emails from
 * the last time that the application was opened. So we say that there
 * are new emails in a folder when the count is higher compared to the
 * last time that the user checked it (count > checkpoint).
 *
 * The global counter tracks the number of folders where count > checkpoint.
 * When there are no longer any folders over the checkpoint, we call a user-
 * provided callback. We also provide a method to set the current count of
 * each folder as its checkpoint, used to set the new acknowledged unread
 * counts per folder.
 *
 * Limitation: We only have per-folder, not per-email granularity. Therefore,
 * we can't know when the folder unread count decreases, if the email that
 * was read was a 'new' one and so we should go ahead and unset the 'unread'
 * status, or if it was an old one which the user already knows about, and
 * we should keep showing the 'unread' status. The current implementation
 * operates according to the 'new' scenario.
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

// Current number of unodes where count > checkpoint
static gint n_folders_over_checkpoint = 0;

// Function to call when n_folders_over_checkpoint reaches 0
static void (*global_checkpoint_reached_cb)(void) = NULL;

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

/* New information regarding the unread count of a folder.
 * - Adjust our internal count record.
 * - Check against our known checkpoint, and update the global record.
 * - If the global record drops to 0, invoke the callback.  */
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
		
		// if was at checkpoint, and now aren't
		if(was_at_checkpoint) {
			n_folders_over_checkpoint++;
			printf("no longer at checkpoint, now have %d folders over checkpoint\n",
				n_folders_over_checkpoint);
		}
	} else if(count < prev_count) {
		if(count <= unode->checkpoint) {
			// can't have count < checkpoint
			unode->checkpoint = count;
			
			if(count < unode->checkpoint)
				printf("checkpoint now lower: %d\n",
					unode->checkpoint);
			
			// if wasn't at checkpoint, but now are
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
	
	// TODO remove
	g_assert(n_folders_over_checkpoint >= 0);
	
	/* Is the new count higher than the previous one? The same? The
	 * negative count is not all that useful, be careful interpreting it. */
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

static void set_checkpoint_foreach_cb(gpointer key, gpointer value,
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
	
	g_hash_table_foreach(utable, set_checkpoint_foreach_cb, NULL);
	n_folders_over_checkpoint = 0;
}
