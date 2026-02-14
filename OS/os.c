#include "os.h"

// BeagleBone Black UART0 base address
#define UART0_BASE     0x44E09000
#define UART_THR       (UART0_BASE + 0x00)       // Transmit Holding Register
#define UART_LSR       (UART0_BASE + 0x14)       // Line Status Register
#define UART_LSR_THRE  0x20                      // Transmit Holding Register Empty
#define UART_LSR_RXFE  0x10                      // Receive FIFO Empty

// BeagleBone Black DMTIMER2 base address
#define DMTIMER2_BASE    0x48040000
#define TCLR             (DMTIMER2_BASE + 0x38)  // Timer Control Register
#define TCRR             (DMTIMER2_BASE + 0x3C)  // Timer Counter Register
#define TISR             (DMTIMER2_BASE + 0x28)  // Timer Interrupt Status Register
#define TIER             (DMTIMER2_BASE + 0x2C)  // Timer Interrupt Enable Register
#define TLDR             (DMTIMER2_BASE + 0x40)  // Timer Load Register

// BeagleBone Black Interrupt Controller (INTCPS) base address
#define INTCPS_BASE      0x48200000
#define INTC_MIR_CLEAR2  (INTCPS_BASE + 0xC8)    // Interrupt Mask Clear Register 2
#define INTC_CONTROL     (INTCPS_BASE + 0x48)    // Interrupt Controller Control
#define INTC_ILR68       (INTCPS_BASE + 0x210)   // Interrupt Line Register 68 (fixed offset from assignment code)

// Clock Manager base address
#define CM_PER_BASE      0x44E00000
#define CM_PER_TIMER2_CLKCTRL (CM_PER_BASE + 0x80)  // Timer2 Clock Control

// ============================================================================
// UART Functions
// ============================================================================

// Function to send a single character via UART
void uart_putc(char c) {
    // Wait until Transmit Holding Register is empty
    while ((GET32(UART_LSR) & UART_LSR_THRE) == 0);
    PUT32(UART_THR, c);
}

// Function to receive a single character via UART
char uart_getc(void) {
    // Wait until data is available
    while ((GET32(UART_LSR) & UART_LSR_RXFE) != 0);
    return (char)(GET32(UART_THR) & 0xFF);
}

// Function to send a string via UART
void os_write(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

// Function to receive a line of input via UART
void os_read(char *buffer, int max_length) {
    int i = 0;
    char c;
    while (i < max_length - 1) { // Leave space for null terminator
        c = uart_getc();
        if (c == '\n' || c == '\r') {
            uart_putc('\n'); // Echo newline
            break;
        }
        uart_putc(c); // Echo character
        buffer[i++] = c;
    }
    buffer[i] = '\0'; // Null terminate the string
}

// Helper function to print an unsigned integer
void uart_putnum(unsigned int num) {
    char buf[10];
    int i = 0;
    if (num == 0) {
        uart_putc('0');
        uart_putc('\n');
        return;
    }
    do {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0 && i < 10);
    while (i > 0) {
        uart_putc(buf[--i]);
    }
    uart_putc('\n');
}

// Function to send a number in hexadecimal via UART
void uart_puthex(unsigned int num) {
    char hex[] = "0123456789ABCDEF";
    int i;
    uart_putc('0');
    uart_putc('x');
    for (i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(num >> i) & 0xF]);
    }
}

// Function to display a label and register value for debugging
void os_debug(const char *s, unsigned int num) {
    os_write(s);
    os_write(": ");
    uart_puthex(num);
    uart_putc('\n');
}

// ============================================================================
// Timer Functions
// ============================================================================

// Timer initialization set for interruptions every 2 seconds
void timer_init(void) {
    // Enable timer
    PUT32(CM_PER_TIMER2_CLKCTRL, 0x2);
    // Unmask interrupt controller
    PUT32(INTC_MIR_CLEAR2, 0x10);
    // Set priority
    PUT32(INTC_ILR68, 0x0);
    // Stop and reset timer
    PUT32(TCLR, 0x0);
    // Clear interrupts
    PUT32(TISR, 0x7);
    // Set timer (2 seconds)
    PUT32(TLDR, 0xFE91CA00);
    // Set counter 
    PUT32(TCRR, 0xFE91CA00);
    // Enable overflow interrupt
    PUT32(TIER, 0x2);
    // Start timer in auto-reload mode
    PUT32(TCLR, 0x3);
}

// Timer interrupt handler. It clears the interrupt flag and responds with a "Tick".
void timer_irq_handler(void) {
    // Clear timer interrupt flag
    PUT32(TISR, 0x2);
    // Set interrupt as handled
    PUT32(INTC_CONTROL, 0x1);
    os_write("Tick\n");
}

// ============================================================================
// Main Program
// ============================================================================

// Simple random number generator (Linear Congruential Generator)
static unsigned int seed = 12345;

unsigned int rand(void) {
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return seed;
}

int main(void) {
    os_write("Starting...\n");
    timer_init();
    os_write("Timer initialized.\n");
    enable_irq();
    os_write("Interrupts enabled.\n");

    // Main loop: continuously print random numbers
    while (1) {
        unsigned int random_num = rand() % 1000;
        uart_putnum(random_num);
        
        // Small delay to prevent overwhelming UART
        for (volatile int i = 0; i < 1000000; i++);
    }
    
    return 0;
}
