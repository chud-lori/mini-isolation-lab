CC ?= cc
CFLAGS ?= -Wall -Wextra -O2
UNAME_S := $(shell uname -s)

.PHONY: all parts check-docs rootfs part-07-container part-09-microvm run-part-06-cgroups run-part-08-kvm run-part-10-microvm-memory run-part-11-hardening run-part-12-userns run-part-13-kvm-capabilities run-part-14-capabilities run-mini-firecracker help clean

all: mini-container mini-firecracker
parts: part-01-syscalls part-02-memory part-03-processes part-04-namespaces part-05-rootfs part-06-cgroups part-07-container part-08-kvm part-09-microvm part-10-microvm-memory part-11-hardening part-12-userns part-13-kvm-capabilities part-14-capabilities

check-docs:
	python3 scripts/check_docs_sources.py

help:
	@echo "Targets:"
	@echo "  make parts                 build tutorial part binaries (Linux only)"
	@echo "  make check-docs            verify docs source panels and source links"
	@echo "  make rootfs                create ./rootfs with BusyBox applets (Linux only)"
	@echo "  make all                   build mini-container and mini-firecracker (Linux only)"
	@echo "  make run-part-06-cgroups   build and run the cgroup v2 limit demo"
	@echo "  make run-part-08-kvm       build and run the KVM API version probe"
	@echo "  make run-part-10-microvm-memory  build and run the KVM memory slot demo"
	@echo "  make run-part-11-hardening build and run the container hardening demo"
	@echo "  make run-part-12-userns    build and run the user namespace mapping demo"
	@echo "  make run-part-13-kvm-capabilities  build and run the KVM capability checklist"
	@echo "  make run-part-14-capabilities  build and run the capability dropping demo"
	@echo "  make run-mini-firecracker  build and run the tiny KVM microVM demo"
	@echo "  make clean                 remove built binaries"

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

part-07-container: mini-container

part-08-kvm: src/part-08-kvm/kvm_api_version.c
	$(CC) $(CFLAGS) -o $@ $<

part-09-microvm: mini-firecracker

part-10-microvm-memory: src/part-10-microvm-memory/kvm_memory_slots.c
	$(CC) $(CFLAGS) -o $@ $<

part-11-hardening: src/part-11-hardening/container_hardening.c
	$(CC) $(CFLAGS) -o $@ $<

part-12-userns: src/part-12-userns/userns_map.c
	$(CC) $(CFLAGS) -o $@ $<

part-13-kvm-capabilities: src/part-13-kvm-capabilities/kvm_capabilities.c
	$(CC) $(CFLAGS) -o $@ $<

part-14-capabilities: src/part-14-capabilities/drop_capabilities.c
	$(CC) $(CFLAGS) -o $@ $<

mini-container: src/part-07-container/mini_container.c
	$(CC) $(CFLAGS) -o $@ $<

mini-firecracker: src/part-09-microvm/mini_firecracker.c
	$(CC) $(CFLAGS) -o $@ $<

rootfs:
	sh scripts/create_busybox_rootfs.sh rootfs

run-part-06-cgroups: part-06-cgroups
	./part-06-cgroups

run-part-08-kvm: part-08-kvm
	./part-08-kvm

run-part-10-microvm-memory: part-10-microvm-memory
	./part-10-microvm-memory

run-part-11-hardening: part-11-hardening
	./part-11-hardening

run-part-12-userns: part-12-userns
	./part-12-userns

run-part-13-kvm-capabilities: part-13-kvm-capabilities
	./part-13-kvm-capabilities

run-part-14-capabilities: part-14-capabilities
	./part-14-capabilities

run-mini-firecracker: mini-firecracker
	./mini-firecracker
else
part-01-syscalls part-02-memory part-03-processes part-04-namespaces part-05-rootfs part-06-cgroups part-07-container part-08-kvm part-09-microvm part-10-microvm-memory part-11-hardening part-12-userns part-13-kvm-capabilities part-14-capabilities:
	@echo "tutorial part binaries require Linux; build them on Linux." >&2
	@false

mini-container:
	@echo "mini-container requires Linux namespaces and cgroups; build it on Linux." >&2
	@false

mini-firecracker:
	@echo "mini-firecracker requires Linux KVM; build and run it on Linux." >&2
	@false

rootfs:
	@echo "rootfs creation expects a Linux BusyBox binary; run it on Linux." >&2
	@false

run-part-06-cgroups:
	@echo "part-06-cgroups requires Linux cgroup v2; build and run it on Linux." >&2
	@false

run-part-08-kvm:
	@echo "part-08-kvm requires Linux KVM; build and run it on Linux." >&2
	@false

run-part-10-microvm-memory:
	@echo "part-10-microvm-memory requires Linux KVM; build and run it on Linux." >&2
	@false

run-part-11-hardening:
	@echo "part-11-hardening requires Linux seccomp; build and run it on Linux." >&2
	@false

run-part-12-userns:
	@echo "part-12-userns requires Linux user namespaces; build and run it on Linux." >&2
	@false

run-part-13-kvm-capabilities:
	@echo "part-13-kvm-capabilities requires Linux KVM; build and run it on Linux." >&2
	@false

run-part-14-capabilities:
	@echo "part-14-capabilities requires Linux capabilities; build and run it on Linux." >&2
	@false

run-mini-firecracker:
	@echo "mini-firecracker requires Linux KVM; build and run it on Linux." >&2
	@false
endif

clean:
	rm -f mini-container mini-firecracker part-01-syscalls part-02-memory \
		part-03-processes part-04-namespaces part-05-rootfs part-06-cgroups \
		part-08-kvm part-10-microvm-memory part-11-hardening part-12-userns \
		part-13-kvm-capabilities part-14-capabilities
