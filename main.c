#include <stm32f2xx.h>
#include <stm32f2xx_gpio.h>
#include <stm32f2xx_spi.h>

__asm__(
"usleep:\n"
"	mov r3, #6\n"
"	muls r0, r3\n"
"1:\n"
"	subs r0, #1\n"
"	bne 1b\n"
"	bx lr\n"
);

extern void usleep(int i);

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


static void display_write(unsigned char c) {
	SPI2->DR = c;
	while (!(SPI2->SR & SPI_SR_TXE))
		;
}

int main() {
	const char *s = "hello, tiny-firmware Pebble!\n";
	
	RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
	puts("hello, tiny-firmware Pebble, from C code!\n");
	
	/* turn on GPIO and SPI clocks */
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	
	/* display:
	 *  display on SPI2
	 *  chip select on GPIOB12
	 *  clock on GPIOB13, data on GPIOB15
	 *  ? on GPIOB14
	 *  VCOM on GPIOB1 (TIM3 ch4)
	 */
	
	/* Set up the GPIOs first. */
	GPIO_InitTypeDef gpioinit;

	GPIO_WriteBit(GPIOB, 1 << 12, 0);
	GPIO_PinAFConfig(GPIOB, 1, GPIO_AF_TIM3);
	GPIO_PinAFConfig(GPIOB, 13, GPIO_AF_SPI2);
	GPIO_PinAFConfig(GPIOB, 15, GPIO_AF_SPI2);
	
	gpioinit.GPIO_Pin = (1 << 12);
	gpioinit.GPIO_Mode = GPIO_Mode_OUT;
	gpioinit.GPIO_Speed = GPIO_Speed_50MHz;
	gpioinit.GPIO_OType = GPIO_OType_PP;
	gpioinit.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &gpioinit);
	
	gpioinit.GPIO_Pin = (1 << 13) | (1 << 15);
	gpioinit.GPIO_Mode = GPIO_Mode_AF;
	gpioinit.GPIO_Speed = GPIO_Speed_50MHz;
	gpioinit.GPIO_OType = GPIO_OType_PP;
	gpioinit.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &gpioinit);
	
	gpioinit.GPIO_Pin = (1 << 1);
	gpioinit.GPIO_Mode = GPIO_Mode_AF;
	gpioinit.GPIO_Speed = GPIO_Speed_50MHz;
	gpioinit.GPIO_OType = GPIO_OType_OD;
	gpioinit.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &gpioinit);
	
	/* Set up the SPI controller, SPI2. */
	SPI_InitTypeDef spiinit;
	
	SPI_I2S_DeInit(SPI2);
	spiinit.SPI_Direction = SPI_Direction_1Line_Tx;
	spiinit.SPI_Mode = SPI_Mode_Master;
	spiinit.SPI_DataSize = SPI_DataSize_8b;
	spiinit.SPI_CPOL = SPI_CPOL_Low;
	spiinit.SPI_CPHA = SPI_CPHA_1Edge;
	spiinit.SPI_NSS = SPI_NSS_Soft;
	spiinit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
	spiinit.SPI_FirstBit = SPI_FirstBit_MSB;
	spiinit.SPI_CRCPolynomial = 7 /* Um. */;
	SPI_Init(SPI2, &spiinit);
	SPI_Cmd(SPI2, ENABLE);
	
	/* XXX: Set up TIM3 for VCOM. */

	puts("here we go on the display\n");
	/* Now scribble something on the display. */
	/* Set the chip select. */
	GPIO_WriteBit(GPIOB, 1 << 12, 1);
	usleep(7);
	
	display_write(0x80);
	for (int i = 1; i <= 168; i++) {
		display_write(__RBIT(__REV(i)));
		for (int j = 0; j < 18; j++) {
			display_write(((i/8 + j) % 2) ? 0xFF : 0x00);
		}
		display_write(0);
	}
	display_write(0);

	/* clear the chip select */
	usleep(7);
	
	GPIO_WriteBit(GPIOB, 1 << 12, 0);
	
	puts("and that's that\n");
	
	while (1)
		;
	
	return 0;
}
