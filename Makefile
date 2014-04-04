# Do not:
# o  use make's built-in rules and variables
#    (this increases performance and avoids hard-to-debug behaviour);
# o  print "Entering directory ...";
.SUFFIXES:
MAKEFLAGS += -rR --no-print-directory

# Avoid funny character set dependencies
unexport LC_ALL
LC_COLLATE=C
LC_NUMERIC=C
export LC_COLLATE LC_NUMERIC

CFLAGS=-Wall -pthread

# By default "make" will build the first target -- here we're just
# calling it "all".
.PHONY: all
all:

# Source each lab's Makefile seperately -- this way we can provide the
# labs to the students one at a time
-include */Makefile.mk

# These are recursive builds that run under different build systems --
# essentially what this means is that we have an entirely different
# build system for these projects, and their build systems are just
# run every time we type "make".
.PHONY: busybox
busybox::
	$(MAKE) -C busybox
all: busybox

.PHONY: linux
linux:
	$(MAKE) -C linux
all: linux

.PHONY: qemu
qemu: qemu/qemu-options.def
	$(MAKE) -C qemu
all: qemu

# Apparently we can't cache configure because it does some dependency
# checking.
qemu/qemu-options.def: qemu/configure
	cd qemu && ./configure --target-list=x86_64-softmmu

# We can't track dependencies through those recursive builds, but we
# can kind of do it -- this always rebuilds the subprojects, but only
# rebuilds these projects when something actually changes in a
# subproject
busybox/busybox: busybox
linux/usr/gen_init_cpio: linux

# Adds a trivial clean target -- this gets most things
.PHONY: clean
clean::
	$(MAKE) -C linux clean
	$(MAKE) -C busybox clean
	$(MAKE) -C qemu clean
	rm -rf .httpd/* .lab0-fish/* .lab0/* .lab0-fish/* .obj/*

# This target runs the tests and reports their results
.PHONY: check
check:
	cat $^
%.cunit_out: %
	$< | tee "$@"
