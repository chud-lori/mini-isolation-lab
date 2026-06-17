CC ?= cc
CFLAGS ?= -Wall -Wextra -O2
UNAME_S := $(shell uname -s)

.PHONY: all parts clean

all: mini-container mini-firecracker
parts: part-01-syscalls part-02-memory part-03-processes part-04-namespaces part-05-rootfs part-06-cgroups part-08-kvm

ifeq ($(UNAME_S),Linux)
part-01-syscalls: src/part-01-syscalls/syscall_write.c
	$(CC) $(CFLAGS) -o $@ $<

part-02-memory: src/part-02-memory/mmap_memory.c
	$(CC) $(CFLAGS) -o $@ $<

part-03-processes: src/part-03-processes/clone_process.c
	$(CC) $(CFLAGS) -o $@ $<

part-04-namespaces: src/part-04-namespaces/uts_namespace.c
	$(CC) $(CFLAGS) -o $@ $<

part-05-rootfs: src/part-05-rootfs/chroot_exec.c
	$(CC) $(CFLAGS) -o $@ $<

part-06-cgroups: src/part-06-cgroups/cgroup_limit.c
	$(CC) $(CFLAGS) -o $@ $<

part-08-kvm: src/part-08-kvm/kvm_api_version.c
	$(CC) $(CFLAGS) -o $@ $<

mini-container: src/part-07-container/mini_container.c
	$(CC) $(CFLAGS) -o $@ $<

mini-firecracker: src/part-09-microvm/mini_firecracker.c
	$(CC) $(CFLAGS) -o $@ $<
else
part-01-syscalls part-02-memory part-03-processes part-04-namespaces part-05-rootfs part-06-cgroups part-08-kvm:
	@echo "tutorial part binaries require Linux; build them on Linux." >&2
	@false

mini-container:
	@echo "mini-container requires Linux namespaces and cgroups; build it on Linux." >&2
	@false

mini-firecracker:
	@echo "mini-firecracker requires Linux KVM; build and run it on Linux." >&2
	@false
endif

clean:
	rm -f mini-container mini-firecracker part-01-syscalls part-02-memory \
		part-03-processes part-04-namespaces part-05-rootfs part-06-cgroups \
		part-08-kvm
