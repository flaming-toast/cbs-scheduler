#include "cbs_snapshot.h"
#include <linux/types.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

struct cbs_snapshot_task snapshot_buffer[SNAPSHOT_BUF_SIZE];

int snapshot_written[SNAP_MAX_TRIGGERS];

snap_event sn_events[SNAP_MAX_TRIGGERS];
snap_trig sn_triggers[SNAP_MAX_TRIGGERS];

struct cbs_snapshot_task history_buffer[CBS_MAX_HISTORY];
int history_buffer_head;

int snapshot(snap_event *events, snap_trig *triggers, size_t n) {
  int i = 0;
  memset(snapshot_buffer, 0, SNAPSHOT_BUF_SIZE * sizeof(struct cbs_snapshot_task));
  memset(snapshot_written, 0, SNAP_MAX_TRIGGERS * sizeof(int));
  memset(sn_events, 0, SNAP_MAX_TRIGGERS * sizeof(snap_event));
  memset(sn_triggers, 0, SNAP_MAX_TRIGGERS * sizeof(snap_trig));
  for (; i < SNAP_MAX_TRIGGERS; i++) {
    sn_events[i] = *(events + i);
    sn_triggers[i] = *(triggers + i);
  }
  return 0;
}

int snap_join(void) {
  return 0;
}

/* The syscall for snapshots. */
long sys_snapshot(long eventp, long trigp, long n) {

  long out;
  snap_event* ev = kmalloc(sizeof(snap_event) * n, GFP_KERNEL);
  snap_trig* tr = kmalloc(sizeof(snap_trig) * n, GFP_KERNEL);
  __copy_from_user(ev, (void*)eventp, sizeof(snap_event) * n);
  __copy_from_user(tr, (void*)trigp, sizeof(snap_trig) * n);
  out = (long) snapshot(ev, tr, n);
  kfree(ev);
  kfree(tr);
  return out;
}
