/* nob_commit_tracker.h
 * Pending/Committed Table Implementation
 */
#ifndef _NOB_COMMIT_TRACKER_H
#define _NOB_COMMIT_TRACKER_H

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/hashtable.h>

#include "ext4_jbd2.h"

#define NOB_HASH_BITS 8

struct nob_pending_entry {
    unsigned long ino;       
    struct hlist_node node;  
};

struct nob_committed_entry {
    unsigned long ino;
    struct hlist_node node;
};

struct nob_commit_tracker {
    DECLARE_HASHTABLE(pending,   NOB_HASH_BITS);
    DECLARE_HASHTABLE(committed, NOB_HASH_BITS);
    spinlock_t lock;
};


struct nob_cb_entry {
	struct ext4_journal_cb_entry jce;
	unsigned long ino;
};

extern struct nob_commit_tracker g_nob_tracker;

void nob_tracker_init(void);
int  nob_pending_add(unsigned long ino);
bool nob_is_committed(unsigned long ino);
void nob_move_to_committed(unsigned long ino);
void nob_committed_erase(unsigned long ino);
bool nob_pending_contains(unsigned long ino);

void nob_commit_callback(struct super_block *sb, struct ext4_journal_cb_entry *jce, int rc);
int nob_register_callback(handle_t *handle, unsigned long ino);

#endif /* _NOB_COMMIT_TRACKER_H */
