#include <kernel/arch.h>
#include <kernel/mem.h>

#include <assert.h>

static uint64_t gdt_entry_bits(const struct kx86_gdt_entry *entry)
{
    uint64_t value = 0;

    assert(kmemcpy(&value, entry, sizeof(*entry)) == &value);
    return value;
}

static void test_gdt_descriptor_layout(void)
{
    assert(sizeof(struct kx86_gdt_entry) == 8);
    assert(sizeof(struct kx86_gdtr) == 10);
    assert(sizeof(struct kx86_idt_gate) == 16);
    assert(sizeof(struct kx86_idtr) == 10);
}

static void test_gdt_null_and_system_builder(void)
{
    struct kx86_gdt_entry entry = kx86_gdt_null_entry();

    assert(gdt_entry_bits(&entry) == 0);

    kx86_gdt_entry_set(&entry, 0x12345678u, 0x000abcdeu, 0x89, 0x04);
    assert(entry.limit_low == 0xbcdeu);
    assert(entry.base_low == 0x5678u);
    assert(entry.base_mid == 0x34u);
    assert(entry.access == 0x89u);
    assert(entry.limit_high_flags == 0x4au);
    assert(entry.base_high == 0x12u);

    kx86_gdt_entry_set(NULL, 0, 0, 0, 0);
}

static void test_gdt_flat_long_mode_segments(void)
{
    struct kx86_gdt_entry kernel_code = kx86_gdt_code_segment(0);
    struct kx86_gdt_entry kernel_data = kx86_gdt_data_segment(0);
    struct kx86_gdt_entry user_code = kx86_gdt_code_segment(3);
    struct kx86_gdt_entry user_data = kx86_gdt_data_segment(3);

    assert(gdt_entry_bits(&kernel_code) == 0x00af9a000000ffffu);
    assert(gdt_entry_bits(&kernel_data) == 0x00cf92000000ffffu);
    assert(gdt_entry_bits(&user_code) == 0x00affa000000ffffu);
    assert(gdt_entry_bits(&user_data) == 0x00cff2000000ffffu);
}

static void test_gdt_default_table(void)
{
    struct kx86_gdt_entry entries[KX86_GDT_DEFAULT_ENTRIES];
    struct kx86_gdt_entry too_small[2] = {
        kx86_gdt_data_segment(0),
        kx86_gdt_data_segment(0),
    };

    kx86_gdt_default_table(entries, KX86_GDT_DEFAULT_ENTRIES);
    assert(gdt_entry_bits(&entries[KX86_GDT_INDEX_NULL]) == 0);
    assert(gdt_entry_bits(&entries[KX86_GDT_INDEX_KERNEL_CODE]) == 0x00af9a000000ffffu);
    assert(gdt_entry_bits(&entries[KX86_GDT_INDEX_KERNEL_DATA]) == 0x00cf92000000ffffu);
    assert(gdt_entry_bits(&entries[KX86_GDT_INDEX_USER_CODE]) == 0x00affa000000ffffu);
    assert(gdt_entry_bits(&entries[KX86_GDT_INDEX_USER_DATA]) == 0x00cff2000000ffffu);

    assert(KX86_SELECTOR_KERNEL_CODE == 0x08u);
    assert(KX86_SELECTOR_KERNEL_DATA == 0x10u);
    assert(KX86_SELECTOR_USER_CODE == 0x1bu);
    assert(KX86_SELECTOR_USER_DATA == 0x23u);

    kx86_gdt_default_table(too_small, sizeof(too_small) / sizeof(too_small[0]));
    assert(gdt_entry_bits(&too_small[0]) == 0x00cf92000000ffffu);
    assert(gdt_entry_bits(&too_small[1]) == 0x00cf92000000ffffu);
    kx86_gdt_default_table(NULL, KX86_GDT_DEFAULT_ENTRIES);
}

static void test_gdtr_builder(void)
{
    struct kx86_gdt_entry entries[] = {
        kx86_gdt_null_entry(),
        kx86_gdt_code_segment(0),
        kx86_gdt_data_segment(0),
    };
    struct kx86_gdtr gdtr = kx86_gdtr_make(entries, sizeof(entries) / sizeof(entries[0]));
    struct kx86_gdtr empty = kx86_gdtr_make(entries, 0);
    struct kx86_gdtr missing = kx86_gdtr_make(NULL, 3);

    assert(gdtr.limit == sizeof(entries) - 1);
    assert(gdtr.base == (uintptr_t)entries);
    assert(empty.limit == 0);
    assert(empty.base == 0);
    assert(missing.limit == 0);
    assert(missing.base == 0);
}

static void test_idt_gate_builder(void)
{
    const uintptr_t handler = (uintptr_t)0x123456789abcdef0ull;
    struct kx86_idt_gate gate = {0};

    kx86_idt_gate_set(&gate, handler, 0x08, 9, KX86_IDT_GATE_INTERRUPT, 3);
    assert(gate.offset_low == 0xdef0u);
    assert(gate.selector == 0x08u);
    assert(gate.ist == 1u);
    assert(gate.type_attr == 0xeeu);
    assert(gate.offset_mid == 0x9abcu);
    assert(gate.offset_high == 0x12345678u);
    assert(gate.reserved == 0);

    kx86_idt_gate_set(NULL, handler, 0x08, 0, KX86_IDT_GATE_INTERRUPT, 0);
}

static void test_idt_gate_helpers(void)
{
    struct kx86_idt_gate interrupt = kx86_idt_interrupt_gate(0xffffffff80001234ull, 0x08);
    struct kx86_idt_gate trap = kx86_idt_trap_gate(0xffffffff80005678ull, 0x1b, 3);

    assert(interrupt.offset_low == 0x1234u);
    assert(interrupt.selector == 0x08u);
    assert(interrupt.ist == 0);
    assert(interrupt.type_attr == 0x8eu);
    assert(interrupt.offset_mid == 0x8000u);
    assert(interrupt.offset_high == 0xffffffffu);
    assert(interrupt.reserved == 0);

    assert(trap.offset_low == 0x5678u);
    assert(trap.selector == 0x1bu);
    assert(trap.type_attr == 0xefu);
    assert(trap.offset_mid == 0x8000u);
    assert(trap.offset_high == 0xffffffffu);
}

static void test_idt_clear(void)
{
    struct kx86_idt_gate gates[3] = {
        kx86_idt_interrupt_gate(0xffffffff80000000ull, KX86_SELECTOR_KERNEL_CODE),
        kx86_idt_interrupt_gate(0xffffffff80000010ull, KX86_SELECTOR_KERNEL_CODE),
        kx86_idt_interrupt_gate(0xffffffff80000020ull, KX86_SELECTOR_KERNEL_CODE),
    };

    kx86_idt_clear(gates, sizeof(gates) / sizeof(gates[0]));
    for (size_t i = 0; i < sizeof(gates) / sizeof(gates[0]); ++i) {
        assert(gates[i].offset_low == 0);
        assert(gates[i].selector == 0);
        assert(gates[i].ist == 0);
        assert(gates[i].type_attr == 0);
        assert(gates[i].offset_mid == 0);
        assert(gates[i].offset_high == 0);
        assert(gates[i].reserved == 0);
    }

    kx86_idt_clear(NULL, 3);
}

static void test_idtr_builder(void)
{
    struct kx86_idt_gate gates[] = {
        kx86_idt_interrupt_gate(0xffffffff80000000ull, 0x08),
        kx86_idt_interrupt_gate(0xffffffff80000010ull, 0x08),
    };
    struct kx86_idtr idtr = kx86_idtr_make(gates, sizeof(gates) / sizeof(gates[0]));
    struct kx86_idtr empty = kx86_idtr_make(gates, 0);
    struct kx86_idtr missing = kx86_idtr_make(NULL, 2);

    assert(idtr.limit == sizeof(gates) - 1);
    assert(idtr.base == (uintptr_t)gates);
    assert(empty.limit == 0);
    assert(empty.base == 0);
    assert(missing.limit == 0);
    assert(missing.base == 0);
}

void test_arch(void)
{
    test_gdt_descriptor_layout();
    test_gdt_null_and_system_builder();
    test_gdt_flat_long_mode_segments();
    test_gdt_default_table();
    test_gdtr_builder();
    test_idt_gate_builder();
    test_idt_gate_helpers();
    test_idt_clear();
    test_idtr_builder();
}
