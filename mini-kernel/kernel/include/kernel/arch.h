#ifndef KERNEL_ARCH_H
#define KERNEL_ARCH_H

#include <kernel/compiler.h>
#include <kernel/types.h>

enum {
    KX86_GDT_DEFAULT_ENTRIES = 5,
    KX86_IDT_DEFAULT_ENTRIES = 256,
};

enum {
    KX86_GDT_INDEX_NULL = 0,
    KX86_GDT_INDEX_KERNEL_CODE = 1,
    KX86_GDT_INDEX_KERNEL_DATA = 2,
    KX86_GDT_INDEX_USER_CODE = 3,
    KX86_GDT_INDEX_USER_DATA = 4,
};

enum {
    KX86_SELECTOR_KERNEL_CODE = KX86_GDT_INDEX_KERNEL_CODE << 3,
    KX86_SELECTOR_KERNEL_DATA = KX86_GDT_INDEX_KERNEL_DATA << 3,
    KX86_SELECTOR_USER_CODE = (KX86_GDT_INDEX_USER_CODE << 3) | 0x03,
    KX86_SELECTOR_USER_DATA = (KX86_GDT_INDEX_USER_DATA << 3) | 0x03,
};

enum {
    KX86_GDT_ACCESS_ACCESSED = 0x01,
    KX86_GDT_ACCESS_READ_WRITE = 0x02,
    KX86_GDT_ACCESS_EXECUTABLE = 0x08,
    KX86_GDT_ACCESS_DESCRIPTOR = 0x10,
    KX86_GDT_ACCESS_PRESENT = 0x80,
};

enum {
    KX86_GDT_FLAG_LONG_MODE = 0x02,
    KX86_GDT_FLAG_32BIT = 0x04,
    KX86_GDT_FLAG_GRANULARITY_4K = 0x08,
};

struct kx86_gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t limit_high_flags;
    uint8_t base_high;
} __attribute__((packed));

struct kx86_gdtr {
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed));

enum {
    KX86_IDT_GATE_INTERRUPT = 0x0e,
    KX86_IDT_GATE_TRAP = 0x0f,
    KX86_IDT_GATE_PRESENT = 0x80,
};

struct kx86_idt_gate {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct kx86_idtr {
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed));

void arch_early_init(void);
KERNEL_NORETURN void arch_halt(void);
void arch_outb(uint16_t port, uint8_t value);
uint8_t arch_inb(uint16_t port);
void arch_serial_init(void);
void arch_serial_putc(char ch);

void kx86_gdt_entry_set(
    struct kx86_gdt_entry *entry,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t flags);
struct kx86_gdt_entry kx86_gdt_null_entry(void);
struct kx86_gdt_entry kx86_gdt_code_segment(uint8_t dpl);
struct kx86_gdt_entry kx86_gdt_data_segment(uint8_t dpl);
void kx86_gdt_default_table(struct kx86_gdt_entry *entries, size_t count);
struct kx86_gdtr kx86_gdtr_make(const struct kx86_gdt_entry *entries, size_t count);
void kx86_idt_gate_set(
    struct kx86_idt_gate *gate,
    uintptr_t handler,
    uint16_t selector,
    uint8_t ist,
    uint8_t gate_type,
    uint8_t dpl);
struct kx86_idt_gate kx86_idt_interrupt_gate(uintptr_t handler, uint16_t selector);
struct kx86_idt_gate kx86_idt_trap_gate(uintptr_t handler, uint16_t selector, uint8_t dpl);
void kx86_idt_clear(struct kx86_idt_gate *gates, size_t count);
struct kx86_idtr kx86_idtr_make(const struct kx86_idt_gate *gates, size_t count);
void kx86_lgdt(const struct kx86_gdtr *gdtr);
void kx86_lidt(const struct kx86_idtr *idtr);
void kx86_descriptor_tables_init(void);

#endif
