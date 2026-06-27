#include <kernel/arch.h>

static uint8_t kx86_gdt_ring_bits(uint8_t dpl)
{
    return (uint8_t)((dpl & 0x03u) << 5);
}

void kx86_gdt_entry_set(
    struct kx86_gdt_entry *entry,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t flags)
{
    if (entry == NULL) {
        return;
    }

    entry->limit_low = (uint16_t)(limit & 0xffffu);
    entry->base_low = (uint16_t)(base & 0xffffu);
    entry->base_mid = (uint8_t)((base >> 16) & 0xffu);
    entry->access = access;
    entry->limit_high_flags = (uint8_t)(((limit >> 16) & 0x0fu) | ((flags & 0x0fu) << 4));
    entry->base_high = (uint8_t)((base >> 24) & 0xffu);
}

struct kx86_gdt_entry kx86_gdt_null_entry(void)
{
    struct kx86_gdt_entry entry = {0};
    return entry;
}

struct kx86_gdt_entry kx86_gdt_code_segment(uint8_t dpl)
{
    struct kx86_gdt_entry entry;
    const uint8_t access = KX86_GDT_ACCESS_PRESENT | kx86_gdt_ring_bits(dpl) |
                           KX86_GDT_ACCESS_DESCRIPTOR | KX86_GDT_ACCESS_EXECUTABLE |
                           KX86_GDT_ACCESS_READ_WRITE;
    const uint8_t flags = KX86_GDT_FLAG_GRANULARITY_4K | KX86_GDT_FLAG_LONG_MODE;

    kx86_gdt_entry_set(&entry, 0, 0x000fffffu, access, flags);
    return entry;
}

struct kx86_gdt_entry kx86_gdt_data_segment(uint8_t dpl)
{
    struct kx86_gdt_entry entry;
    const uint8_t access = KX86_GDT_ACCESS_PRESENT | kx86_gdt_ring_bits(dpl) |
                           KX86_GDT_ACCESS_DESCRIPTOR | KX86_GDT_ACCESS_READ_WRITE;
    const uint8_t flags = KX86_GDT_FLAG_GRANULARITY_4K | KX86_GDT_FLAG_32BIT;

    kx86_gdt_entry_set(&entry, 0, 0x000fffffu, access, flags);
    return entry;
}

void kx86_gdt_default_table(struct kx86_gdt_entry *entries, size_t count)
{
    if (entries == NULL || count < KX86_GDT_DEFAULT_ENTRIES) {
        return;
    }

    entries[KX86_GDT_INDEX_NULL] = kx86_gdt_null_entry();
    entries[KX86_GDT_INDEX_KERNEL_CODE] = kx86_gdt_code_segment(0);
    entries[KX86_GDT_INDEX_KERNEL_DATA] = kx86_gdt_data_segment(0);
    entries[KX86_GDT_INDEX_USER_CODE] = kx86_gdt_code_segment(3);
    entries[KX86_GDT_INDEX_USER_DATA] = kx86_gdt_data_segment(3);
}

struct kx86_gdtr kx86_gdtr_make(const struct kx86_gdt_entry *entries, size_t count)
{
    if (entries == NULL || count == 0) {
        return (struct kx86_gdtr){0};
    }

    return (struct kx86_gdtr){
        .limit = (uint16_t)(count * sizeof(entries[0]) - 1u),
        .base = (uintptr_t)entries,
    };
}
