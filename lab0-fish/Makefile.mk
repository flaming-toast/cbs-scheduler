# Builds an initrd that runs a userspace version of the fish test --
# this still writes to VGA memory, it just doesn't run the fish system
# calls in kernel space
all: .lab0-fish/user_initrd.gz
.lab0-fish/user_initrd.gz: lab0-fish/user_config .lab0-fish/user_init \
                      linux/usr/gen_init_cpio lab0-fish/frame*.txt
	linux/usr/gen_init_cpio "$<" | gzip > "$@"

.lab0-fish/user_init: lab0-fish/test_harness.c lab0-fish/fish_impl.h \
		 lab0-fish/fish_impl.c lab0-fish/fish_compat.h \
		 lab0-fish/fish_syscall.h lab0-fish/fish_syscall.c
	$(CC) $(CFLAGS) -static -DUSERSPACE_TEST $^ -o "$@"

# Builds an initrd that runs a kernelspace version of the fish test
all: .lab0-fish/kernel_initrd.gz
.lab0-fish/kernel_initrd.gz: lab0-fish/kernel_config .lab0-fish/kernel_init \
                        linux/usr/gen_init_cpio lab0-fish/frame*.txt
	linux/usr/gen_init_cpio "$<" | gzip > "$@"

.lab0-fish/kernel_init: lab0-fish/test_harness.c lab0-fish/fish_impl.h \
                   lab0-fish/fish_syscall.h lab0-fish/fish_syscall.c
	$(CC) $(CFLAGS) -static $^ -o "$@"

# Enforces that some of these files are copied into the kernel 
linux: linux/drivers/gpu/vga/fish_impl.c
linux/drivers/gpu/vga/fish_impl.c: lab0-fish/fish_impl.c
	cp "$<" "$@"
linux: linux/drivers/gpu/vga/fish_impl.h
linux/drivers/gpu/vga/fish_impl.h: lab0-fish/fish_impl.h
	cp "$<" "$@"
linux: linux/drivers/gpu/vga/sys_fish.c
linux/drivers/gpu/vga/sys_fish.c: lab0-fish/sys_fish.c
	cp "$<" "$@"
linux: linux/drivers/gpu/vga/fish_compat.h
linux/drivers/gpu/vga/fish_compat.h: lab0-fish/fish_compat.h
	cp "$<" "$@"
