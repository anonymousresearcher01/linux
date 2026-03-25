/* nob_commit_tracker.c */
#include "nob_commit_tracker.h"
#include <linux/slab.h>
#include <linux/printk.h>
#include "ext4_jbd2.h"
#include <linux/fs.h>

struct nob_commit_tracker g_nob_tracker;

void nob_tracker_init(void)
{
    hash_init(g_nob_tracker.pending);
    hash_init(g_nob_tracker.committed);
    spin_lock_init(&g_nob_tracker.lock);
    pr_info("NobLSM: commit tracker initialized\n");
}

int nob_pending_add(unsigned long ino)
{
    struct nob_pending_entry *e;
	pr_info("NobLSM: pending_add inode %lu\n", ino);

    e = kmalloc(sizeof(*e), GFP_KERNEL);
    if (!e)
        return -ENOMEM;

    e->ino = ino;
    spin_lock(&g_nob_tracker.lock);
    hash_add(g_nob_tracker.pending, &e->node, ino);
    spin_unlock(&g_nob_tracker.lock);
    return 0;
}

bool nob_is_committed(unsigned long ino)
{
    struct nob_committed_entry *e;
    bool found = false;

    spin_lock(&g_nob_tracker.lock);
    hash_for_each_possible(g_nob_tracker.committed, e, node, ino) {
        if (e->ino == ino) {
            found = true;
            break;
        }
    }
    spin_unlock(&g_nob_tracker.lock);
    return found;
}

void nob_move_to_committed(unsigned long ino)
{
    struct nob_pending_entry *pe;
    struct nob_committed_entry *ce;

    spin_lock(&g_nob_tracker.lock);

    hash_for_each_possible(g_nob_tracker.pending, pe, node, ino) {
        if (pe->ino == ino) {
            hash_del(&pe->node);
            kfree(pe);
            break;
        }
    }

    spin_unlock(&g_nob_tracker.lock);

    ce = kmalloc(sizeof(*ce), GFP_KERNEL);
    if (!ce)
        return;

    ce->ino = ino;
    spin_lock(&g_nob_tracker.lock);
    hash_add(g_nob_tracker.committed, &ce->node, ino);
    spin_unlock(&g_nob_tracker.lock);
}

void nob_committed_erase(unsigned long ino)
{
    struct nob_committed_entry *e;

    spin_lock(&g_nob_tracker.lock);
    hash_for_each_possible(g_nob_tracker.committed, e, node, ino) {
        if (e->ino == ino) {
            hash_del(&e->node);
            kfree(e);
            break;
        }
    }
    spin_unlock(&g_nob_tracker.lock);
}
bool nob_pending_contains(unsigned long ino)
{
    struct nob_pending_entry *e;
    bool found = false;

    spin_lock(&g_nob_tracker.lock);
    hash_for_each_possible(g_nob_tracker.pending, e, node, ino) {
        if (e->ino == ino) {
            found = true;
            break;
        }
    }
    spin_unlock(&g_nob_tracker.lock);
	pr_info("NobLSM: pending_contains check inode %lu, found=%d\n", ino, found);
    return found;
}
/* commit complete then callback trigger */
void nob_commit_callback(struct super_block *sb,
                         struct ext4_journal_cb_entry *jce,
                         int rc)
{
    struct nob_cb_entry *nce = container_of(jce, struct nob_cb_entry, jce);
    pr_info("NobLSM: commit callback called for inode %lu, rc=%d\n", nce->ino, rc);
    if (rc == 0) {
        /* commit success*/
        nob_move_to_committed(nce->ino);
        pr_debug("NobLSM: inode %lu committed\n", nce->ino);
    }
    kfree(nce);
}

int nob_register_callback(handle_t *handle, unsigned long ino)
{
    struct nob_cb_entry *nce;

    nce = kmalloc(sizeof(*nce), GFP_KERNEL);
    if (!nce)
        return -ENOMEM;

    nce->ino = ino;

    nob_pending_add(ino);

    ext4_journal_callback_add(handle, nob_commit_callback, &nce->jce);
    return 0;
}
