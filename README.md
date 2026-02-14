# BeagleBone Black Bare-Metal OS

A bare-metal interrupt handling program for the BeagleBone Black (AM335x Cortex-A8) that demonstrates interrupt-driven timer operation alongside UART serial communication. The system configures DMTIMER2 to fire an IRQ every ~2 seconds while the main loop continuously outputs random numbers over UART0.

## Added Features

### Interrupt Enabling

IRQ interrupts are enabled at the CPU level by clearing the I-bit (bit 7) in the ARM CPSR (Current Program Status Register). This is done in assembly via the `enable_irq` routine in `OS/root.s`. While the I-bit is set, the processor masks all IRQ signals regardless of the interrupt controller configuration.

### Interrupt Handling

The ARM exception vector table is installed at boot via the VBAR (Vector Base Address Register). When an IRQ fires, the processor branches to `irq_handler` in `OS/root.s`, which saves all general-purpose registers and the link register, calls the C-level handler (`timer_irq_handler`), restores the registers, and returns from the exception with the standard `subs pc, lr, #4` adjustment for the ARM pipeline.

### Timer Configuration

DMTIMER2 is configured to generate an overflow interrupt every ~2 seconds. The setup involves:

1. Enabling the timer clock module via the Clock Manager (`CM_PER_TIMER2_CLKCTRL`).
2. Unmasking IRQ 68 (DMTIMER2) in the interrupt controller's `INTC_MIR_CLEAR2` register.
3. Setting IRQ 68's priority and routing via `INTC_ILR68`.
4. Loading the timer with a reload value of `0xFE91CA00` (~2 second period at 24 MHz).
5. Enabling the overflow interrupt in `TIER` and starting the timer in auto-reload mode via `TCLR`.

## Functions Implemented

### `OS/root.s`

- **`enable_irq`** — Enables IRQ interrupts at the CPU level by reading the CPSR, clearing the I-bit (bit 7), and writing it back. After this call, the processor will respond to unmasked IRQ signals from the interrupt controller.
- **`irq_handler`** — The IRQ exception handler. Saves registers `r0-r12` and `lr` onto the stack, calls `timer_irq_handler` in C to service the interrupt, restores the saved registers, and returns from the exception using `subs pc, lr, #4`.

### `OS/os.c`

- **`timer_init`** — Configures DMTIMER2 for periodic interrupts. Enables the timer clock, unmasks IRQ 68 in the interrupt controller, sets interrupt priority, loads the timer with a ~2 second overflow period, enables the overflow interrupt, and starts the timer in auto-reload mode.
- **`timer_irq_handler`** — Services the DMTIMER2 interrupt. Clears the overflow flag in `TISR`, acknowledges the interrupt to the controller via `INTC_CONTROL`, and prints "Tick" over UART.
- **`uart_puthex`** — Sends a 32-bit unsigned integer over UART in hexadecimal format with a `0x` prefix (e.g., `0x48200210`). Iterates from the most significant nibble to the least, converting each to its ASCII hex character.
- **`os_debug`** — Debug utility that prints a label string followed by a 32-bit value in hexadecimal (e.g., `TIER: 0x00000002`). Combines `os_write`, `uart_puthex`, and a trailing newline to display register contents for runtime diagnostics.

## Main Function

The `main` function in `OS/os.c` initializes the system and enters an infinite loop. It first prints a startup message over UART, then calls `timer_init` to configure DMTIMER2 for periodic interrupts, followed by `enable_irq` to unmask IRQs at the CPU level. Once interrupts are active, the main loop continuously generates pseudo-random numbers using a linear congruential generator, prints them in decimal via `uart_putnum`, and busy-waits briefly to avoid overwhelming the UART. Meanwhile, every ~2 seconds the timer interrupt fires asynchronously, printing "Tick" in between the random number output.