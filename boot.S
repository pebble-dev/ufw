	.cpu cortex-m3
	.syntax unified
	.code 16

	.section .start
	

	.word 0x20001000
	.word start + 1
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0x4e65576f

	.text
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

start:
	@@@ XXX: FIRST ORDER OF BUSINESS MUST BE TO SET UP THE WATCHDOG
	@@@ TIMER, and to check for a button press to crash back into PRF! 
	@@@ Let's try NOT to brick any watches, if we can avoid it.
	
	ldr r0, =RCC_APB1ENR
	ldr r1, [r0]
	orr r1, r1, #RCC_APB1ENR_UART3
	str r1, [r0]
	
	ldr r0, =(UARTDBG + UART_DR)
	ldr r2, =str
1:	ldrb r1, [r2], #1
	strb r1, [r0]
	cmp r1, #0
	it ne
	bne 1b
1:	wfi
	b 1b

str:	.ascii "hello, tiny-firmware pebble!\n\0"