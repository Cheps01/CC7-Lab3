# BeagleBone Black Bare-Metal OS

A bare-metal program for the BeagleBone Black (AM335x Cortex-A8) that demonstrates interrupt-driven timer operation with UART serial communication. DMTIMER2 fires an IRQ every ~2 seconds while the main loop outputs random numbers over UART0.

## Added Features

### Interrupt Enabling

IRQ interrupts are enabled by clearing the I-bit (bit 7) in the ARM CPSR. This is done in assembly via `enable_irq` in `OS/root.s`. While the I-bit is set, the processor masks all IRQ signals.

### Interrupt Handling

The exception vector table is installed at boot via the VBAR register. When an IRQ fires, the processor branches to `irq_handler` in `OS/root.s`, which saves registers, calls the C-level handler (`timer_irq_handler`), restores registers, and returns with `subs pc, lr, #4`.

### Timer Configuration

DMTIMER2 is configured to generate an overflow interrupt every ~2 seconds. The setup involves:

1. Enabling the timer clock module via `CM_PER_TIMER2_CLKCTRL`.
2. Unmasking IRQ 68 (DMTIMER2) in `INTC_MIR_CLEAR2`.
3. Setting IRQ 68's priority and routing via `INTC_ILR68`.
4. Loading the timer with a reload value of `0xFE91CA00` (~2 second period at 24 MHz).
5. Enabling the overflow interrupt in `TIER` and starting the timer in auto-reload mode via `TCLR`.

## Functions Implemented

### `OS/root.s`

- **`enable_irq`** — Enables IRQ interrupts by reading the CPSR, clearing the I-bit (bit 7), and writing it back.
- **`irq_handler`** — Saves registers `r0-r12` and `lr`, calls `timer_irq_handler` in C, restores registers, and returns from the exception.

### `OS/os.c`

- **`timer_init`** — Configures DMTIMER2 for periodic interrupts: enables the clock, unmasks IRQ 68, sets priority, loads a ~2 second overflow period, and starts the timer in auto-reload mode.
- **`timer_irq_handler`** — Clears the overflow flag in `TISR`, acknowledges the interrupt via `INTC_CONTROL`, and prints "Tick" over UART.
- **`uart_puthex`** — Sends a 32-bit unsigned integer over UART in hexadecimal with a `0x` prefix, iterating from the most significant nibble to the least.
- **`os_debug`** — Prints a label followed by a 32-bit value in hexadecimal (e.g., `TIER: 0x00000002`) for runtime register diagnostics.

## Main Function

The `main` function initializes the system and enters an infinite loop. It prints a startup message, calls `timer_init` to configure DMTIMER2, then `enable_irq` to unmask IRQs. The main loop generates pseudo-random numbers, prints them in decimal via `uart_putnum`, and busy-waits briefly. Every ~2 seconds the timer interrupt fires asynchronously, printing "Tick" in between the random number output.
