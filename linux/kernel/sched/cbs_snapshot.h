/* cs194-24 Lab 2 */

#ifndef SNAPSHOT_H
#define SHAPSHOT_H

#include <linux/types.h>

/*
 * The maximum number of snapshots triggers that can be passed to the
 * snapshot interface.
 */
#define SNAP_MAX_TRIGGERS 8
#define CBS_MAX_HISTORY 64

struct cbs_snapshot_task {
  long pid;
  long time_len;
};

#define SNAPSHOT_BUF_SIZE (130 * SNAP_MAX_TRIGGERS)

typedef int snap_event;
typedef int snap_trig;
// Triggers when the CFS scheduler context switches a task
#define SNAP_EVENT_CFS_SCHED 0
#define SNAP_EVENT_CBS_SCHED 1
// Triggers on the edge before an event starts
#define SNAP_TRIG_BEDGE 0
// Triggers on the edge after an event starts
#define SNAP_TRIG_AEDGE 1

extern struct cbs_snapshot_task snapshot_buffer[SNAPSHOT_BUF_SIZE];

extern int snapshot_written[SNAP_MAX_TRIGGERS];

extern snap_event sn_events[SNAP_MAX_TRIGGERS];
extern snap_trig sn_triggers[SNAP_MAX_TRIGGERS];

extern struct cbs_snapshot_task history_buffer[CBS_MAX_HISTORY];
extern int history_buffer_head;


/*
 * Generates a set of snapshots
 *
 * These snapshots end up in staticly allocated kernel buffers, so
 * there is a maximum number of events you can ask for at once.
 *
 * events         An array of length "n" of the events to trigger on
 *
 * triggers       An array of length "n" of the trigger types
 *
 * n              The length of those arrays
 *
 * return         0 on success, -1 on failure.  This call is non-blocking
 *
 * EINVAL         The kernel cannot take "n" snapshots at once.
 */
int snapshot(snap_event *events, snap_trig *triggers, size_t n);

/*
 * Waits for the previous set of snapshots to complete
 *
 * return         0 on success, -1 on failure
 */
int snap_join(void);

#endif
