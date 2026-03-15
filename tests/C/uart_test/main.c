#define UART_TX (*(volatile unsigned char *)0x40008000)

void main(void) {
    char *msg = "Hello from UART!\n";
    while (*msg) {
        UART_TX = *msg++;
    }
    asm volatile ("ecall");
}
