#include <kernel/arch.h>

static uint8_t kx86_idt_ring_bits(uint8_t dpl)
{
    return (uint8_t)((dpl & 0x03u) << 5);
}

void kx86_idt_gate_set(
    struct kx86_idt_gate *gate,
    uintptr_t handler,
    uint16_t selector,
    uint8_t ist,
    uint8_t gate_type,
    uint8_t dpl)
{
    if (gate == NULL) {
        return;
    }

    gate->offset_low = (uint16_t)(handler & 0xffffu);
    gate->selector = selector;
    gate->ist = (uint8_t)(ist & 0x07u);
    gate->type_attr = (uint8_t)(KX86_IDT_GATE_PRESENT | kx86_idt_ring_bits(dpl) | (gate_type & 0x0fu));
    gate->offset_mid = (uint16_t)((handler >> 16) & 0xffffu);
    gate->offset_high = (uint32_t)((handler >> 32) & 0xffffffffu);
    gate->reserved = 0;
}

struct kx86_idt_gate kx86_idt_interrupt_gate(uintptr_t handler, uint16_t selector)
{
    struct kx86_idt_gate gate;
    kx86_idt_gate_set(&gate, handler, selector, 0, KX86_IDT_GATE_INTERRUPT, 0);
    return gate;
}

struct kx86_idt_gate kx86_idt_trap_gate(uintptr_t handler, uint16_t selector, uint8_t dpl)
{
    struct kx86_idt_gate gate;
    kx86_idt_gate_set(&gate, handler, selector, 0, KX86_IDT_GATE_TRAP, dpl);
    return gate;
}

void kx86_idt_clear(struct kx86_idt_gate *gates, size_t count)
{
    if (gates == NULL) {
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        gates[i] = (struct kx86_idt_gate){0};
    }
}

struct kx86_idtr kx86_idtr_make(const struct kx86_idt_gate *gates, size_t count)
{
    if (gates == NULL || count == 0) {
        return (struct kx86_idtr){0};
    }

    return (struct kx86_idtr){
        .limit = (uint16_t)(count * sizeof(gates[0]) - 1u),
        .base = (uintptr_t)gates,
    };
}
