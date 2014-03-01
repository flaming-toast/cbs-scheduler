# Builds a small, interactive shell that you can play with
all: .obj/initrd.gz

.obj/initrd.gz: fs/config busybox/busybox \
                linux/usr/gen_init_cpio fs/init
	linux/usr/gen_init_cpio "$<" | gzip > "$@"
