# Builds a small, interactive shell that you can play with
all: .lab0/interactive_initrd.gz

.lab0/interactive_initrd.gz: lab0/interactive_config busybox/busybox \
                             linux/usr/gen_init_cpio lab0/interactive_init
	linux/usr/gen_init_cpio "$<" | gzip > "$@"

# This init just prints out the kernel version and then promptly exits
all: .lab0/version_initrd.gz

.lab0/version_initrd.gz: lab0/version_config busybox/busybox \
                         linux/usr/gen_init_cpio lab0/version_init
	linux/usr/gen_init_cpio "$<" | gzip > "$@"
