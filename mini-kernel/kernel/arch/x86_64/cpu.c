#include <kernel/arch.h>

static struct kx86_gdt_entry boot_gdt[KX86_GDT_DEFAULT_ENTRIES];
static struct kx86_idt_gate boot_idt[KX86_IDT_DEFAULT_ENTRIES];

void arch_early_init(void)
{
    kx86_descriptor_tables_init();
    arch_serial_init();
}

void kx86_lgdt(const struct kx86_gdtr *gdtr)
{
    if (gdtr == NULL) {
        return;
    }
    __asm__ volatile("lgdt %0" : : "m"(*gdtr) : "memory");
}

void kx86_lidt(const struct kx86_idtr *idtr)
{
    if (idtr == NULL) {
        return;
    }
    __asm__ volatile("lidt %0" : : "m"(*idtr) : "memory");
}

void kx86_descriptor_tables_init(void)
{
    kx86_gdt_default_table(boot_gdt, KX86_GDT_DEFAULT_ENTRIES);
    kx86_idt_clear(boot_idt, KX86_IDT_DEFAULT_ENTRIES);

    struct kx86_gdtr gdtr = kx86_gdtr_make(boot_gdt, KX86_GDT_DEFAULT_ENTRIES);
    struct kx86_idtr idtr = kx86_idtr_make(boot_idt, KX86_IDT_DEFAULT_ENTRIES);

    kx86_lgdt(&gdtr);
    kx86_lidt(&idtr);
}

void arch_outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

uint8_t arch_inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

void arch_halt(void)
{
    for (;;) {
        __asm__ volatile("cli; hlt" : : : "memory");
    }
}
