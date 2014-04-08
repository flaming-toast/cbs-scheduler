# A simple real-time application that communicates over shared memory
# to a controlling process.
PROJECT_DIR  := /home/user/group/student-group-3
REALTIME_SRC := ./realtime/cbs.c ./realtime/rt.c
REALTIME_HDR := $(wildcard ./realtime/*.h)
REALTIME_OBJ := $(REALTIME_SRC:%.c=%.o)
REALTIME_OBJ := $(REALTIME_OBJ:./realtime/%=./.obj/realtime.d/%)
REALTIME_DEP := $(REALTIME_OBJ:%.o:%.d)
BOGO_MIPS    := $(shell cat /proc/cpuinfo  | grep bogomips | head -n1 | cut -d ':' -f2 | sed s/\ //g)
REALTIME_FLAGS := -pthread -DBOGO_MIPS=$(BOGO_MIPS)

-include $(REALTIME_DEP) 

all: .obj/realtime
.obj/realtime: $(REALTIME_OBJ)
	gcc -static -g $(REALTIME_FLAGS) $(CFLAGS) -o "$@" $^ -lrt

.obj/realtime.d/%.o : realtime/%.c $(REALTIME_HDR)
	mkdir -p `dirname $@`
	gcc -g -c -o $@ $(REALTIME_FLAGS) $(CFLAGS) -MD -MP -MF ${@:.o=.d} $<

# A simple real-time application that communicates over shared memory
# to a controlling process.
REALTIMECTL_SRC := ./realtime/ctl.c
REALTIMECTL_HDR := $(wildcard ./realtimectl/*.h)
REALTIMECTL_OBJ := $(REALTIMECTL_SRC:%.c=%.o)
REALTIMECTL_OBJ := $(REALTIMECTL_OBJ:./realtime/%=./.obj/realtimectl.d/%)
REALTIMECTL_DEP := $(REALTIMECTL_OBJ:%.o:%.d)
REALTIMECTL_FLAGS := -pthread

-include $(REALTIMECTL_DEP) 

all: .obj/realtimectl
.obj/realtimectl: $(REALTIMECTL_OBJ)
	gcc -static -g $(REALTIMECTL_FLAGS) $(CFLAGS) -o "$@" $^

.obj/realtimectl.d/%.o : realtime/%.c $(REALTIMECTL_HDR)
	mkdir -p `dirname $@`
	gcc -g -c -o $@ $(REALTIMECTL_FLAGS) $(CFLAGS) -MD -MP -MF ${@:.o=.d} $<

all: install_cbs_proc
install_cbs_proc:
	cp -u realtime/cbs_proc.h linux/kernel/sched
	cp -u realtime/cbs_proc_impl.c linux/kernel/sched
	cp -u realtime/cbs.Makefile linux/kernel/sched/Makefile
	cp -u realtime/snapshot.h linux/kernel/sched/cbs_snapshot.h

setsched:
#	gcc  -I$(PROJECT_DIR)/linux/include/  -g -o realtime/setsched realtime/setsched.c
	gcc -g -static -o realtime/setsched realtime/setsched.c
	linux/usr/gen_init_cpio fs/config | gzip > .obj/initrd.gz
#	p-I$(PROJECT_DIR)/linux/include/linux/
# 	-I$(PROJECT_DIR)/linux/arch/x86/include/ 
