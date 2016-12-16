#define UART_SR_TXE (1 << 7)
#define UART_SR (0x00)
#define UART_DR (0x04)
#define UART1 0x40011000
#define UART2 0x40004400
#define UART3 0x40004800
#define UARTDBG UART3

#define RCC 0x40023800
#define RCC_APB1ENR (RCC + 0x40)
#define RCC_APB1ENR_UART3 (1 << 18)

int putchar(int c) {
	while (!(*(unsigned char *)(UARTDBG + UART_SR) & UART_SR_TXE))
		; /* we are efficient, here! */
	*(unsigned char *)(UARTDBG + UART_DR) = (unsigned char)c;
	
	return 0;
}

const unsigned char *puts(const unsigned char *_s) {
	const unsigned char *s;
	for (s = _s; *s; s++)
		putchar(*s);
	return _s;
}

int main() {
	const char *s = "hello, tiny-firmware Pebble!\n";
	
	*(unsigned long *)RCC_APB1ENR |= RCC_APB1ENR_UART3;
	puts("hello, tiny-firmware Pebble, from C code!\n");
	
	return 0;
}
