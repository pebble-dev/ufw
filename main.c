#include <stm32f2xx.h>

int putchar(int c) {
	while (!(USART3->SR & USART_SR_TXE))
		; /* we are efficient, here! */
	USART3->DR = (unsigned char) c;
	
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
	
	RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
	puts("hello, tiny-firmware Pebble, from C code!\n");
	
	return 0;
}
