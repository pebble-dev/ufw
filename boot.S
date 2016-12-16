	.cpu cortex-m3
	.syntax unified
	.code 16

	.section .start
	
	.word _stack_top
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
	@ relocate .data to RAM
	ldr r0, =_data_vma
	ldr r1, =_data_lma
	ldr r2, =_data_size
	lsr r2, r2, #2
	
1:	cmp r2, #0
	it eq
	beq 2f
	ldr r3, [r1], #4
	str r3, [r0], #4
	add r2, r2, -1
	b 1b
2:

	@ zero out bss
	ldr r0, =_bss
	ldr r2, =_bss_size
	lsr r2, r2, #2
	mov r3, #0
1:	cmp r2, #0
	it eq
	beq 2f
	str r3, [r0], #4
	add r2, r2, -1
	b 1b
2:
	
	@ set up the stack
	ldr sp, =_stack_top
	
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

	.data
str:	.ascii "hello, tiny-firmware pebble!\n\0"
	
	.bss
_stack_bottom:
	.skip 4096
_stack_top:
