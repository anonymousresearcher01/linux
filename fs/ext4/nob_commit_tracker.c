#include "nob_commit_tracker.h"
#include <linux/printk.h>
#include <linux/slab.h>


struct nob_commit_tracker g_nob_tracker;

void nob_tracker_init(void){
    hash_init(g_nob_tracker.pending);
    hash_init(g_nob_tracker.committed);
    spin_lock_init(&g_nob_tracker.spinlock);
    pr_info("NobLSM's commit tracker init\n");
}

int nob_pending_add(unsigned long ino){
    struct nob_pending_entry *entry;
    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry)
        return -ENOMEM;
    
    entry->ino = ino;
    spin_lock(&g_nob_tracker.spinlock);
    hash_add(g_nob_tracker.pending, &entry->node, ino);
    spin_unlock(&g_nob_tracker.spinlock);
    return 0;
}

bool nob_is_committed(unsigned long ino){
    struct nob_committed_entry * entry;
    bool found = false;

    spin_lock(&g_nob_tracker.spinlock);
    hash_for_each_possible(g_nob_tracker.committed, entry, node, ino){
        if (entry->ino == ino) {
            found = true;
            break;
        }
    }
    spin_unlock(&g_nob_tracker.spinlock);
    return found;
}
void nob_move_to_committed(unsigned long ino){
    struct nob_pending_entry *pe;
    struct nob_committed_entry *ce;
    

    spin_lock(&g_nob_tracker.spinlock);
    hash_for_each_possible(g_nob_tracker.pending, pe, node, ino) {
        if (pe->ino == ino){
            hash_del(&pe->node);
            kfree(pe);
            break;
        }
    }
    spin_unlock(&g_nob_tracker.spinlock);
    ce = kmalloc(sizeof(*ce), GFP_KERNEL);
    if (!ce)
        return;
    
    ce->ino = ino;
    spin_lock(&g_nob_tracker.spinlock);
    hash_add(g_nob_tracker.committed, &ce->node, ino);
    spin_unlock(&g_nob_tracker.spinlock);

}

void nob_commiited_erase(unsigned long ino)
{
    struct nob_committed_entry *entry;
    spin_lock(&g_nob_tracker.spinlock);
    hash_for_each_possible(g_nob_tracker.committed, entry, node, ino) {
        if (entry->ino == ino){
            hash_del(&entry->node);
            kfree(entry);
            break;
        }
    }
    spin_unlock(&g_nob_tracker.spinlock);
}


