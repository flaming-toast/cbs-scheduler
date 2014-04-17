/* cs194-24 Lab 2 */

#include "cbs_proc.h"
#include <string.h>
#include <stdio.h>

FILE* get_snap_ptr(int sid) {
  char fileName[17];
  strncpy(fileName, "/proc/snapshot/", 15);
  fileName[15] = (char)(sid + 48);
  fileName[16] = 0;
  return fopen(fileName, "r");
}

void cbs_list_history(int sid, cbs_func_t func, void* arg) { 
  int i;
  if (sid >= 0 && sid < SNAP_MAX_TRIGGERS) {
    FILE* snap_ptr = get_snap_ptr(sid);
    struct cbs_snapshot_task history_buf[CBS_MAX_HISTORY];
    fread(history_buf, sizeof(struct cbs_snapshot_task), CBS_MAX_HISTORY, snap_ptr);
    for (i = 0; (i < CBS_MAX_HISTORY) && (history_buf[i].pid != 0); i++) {
      func((cbs_proc_t) &(history_buf[i]), arg);
    } 
    fclose(snap_ptr);
  }
}

void cbs_list_rq(int sid, cbs_func_t func, void* arg) {
  int i;
  if (sid >= 0 && sid < SNAP_MAX_TRIGGERS) {
    FILE* snap_ptr = get_snap_ptr(sid);
    struct cbs_snapshot_task rq_buf[CBS_MAX_HISTORY];
    fseek(snap_ptr, sizeof(struct cbs_snapshot_task) * (CBS_MAX_HISTORY + 2), SEEK_SET);
    fread(rq_buf, sizeof(struct cbs_snapshot_task), CBS_MAX_HISTORY, snap_ptr);
    for (i = 0; (i < CBS_MAX_HISTORY) && (rq_buf[i].pid != 0); i++) {
      func((cbs_proc_t) &(rq_buf[i]), arg);
    } 
    fclose(snap_ptr);
  }
}

void cbs_list_current(int sid, cbs_func_t func, void* arg) {
  if (sid >= 0 && sid < SNAP_MAX_TRIGGERS) {
    FILE* snap_ptr = get_snap_ptr(sid);
    struct cbs_snapshot_task cur_buf[1];
    fseek(snap_ptr, sizeof(struct cbs_snapshot_task) * CBS_MAX_HISTORY, SEEK_SET);
    fread(cur_buf, sizeof(struct cbs_snapshot_task), 1, snap_ptr);
    func((cbs_proc_t) cur_buf, arg);
    fclose(snap_ptr);
  }
}

void cbs_list_next(int sid, cbs_func_t func, void* arg) {
  if (sid >= 0 && sid < SNAP_MAX_TRIGGERS) {
    FILE* snap_ptr = get_snap_ptr(sid);
    struct cbs_snapshot_task next_buf[1];
    fseek(snap_ptr, sizeof(struct cbs_snapshot_task) * (CBS_MAX_HISTORY + 1), SEEK_SET);
    fread(next_buf, sizeof(struct cbs_snapshot_task), 1, snap_ptr);
    func((cbs_proc_t) next_buf, arg);
    fclose(snap_ptr);
  }
}

