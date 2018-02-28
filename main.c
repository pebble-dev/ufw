#include <stm32f2xx.h>
#include <stm32f2xx_gpio.h>
#include <stm32f2xx_rcc.h>
#include <stm32f2xx_spi.h>
#include <minilib.h>

__asm__(
"usleep:\n"
"	mov r3, #6\n"
"	muls r0, r3\n"
"1:\n"
"	subs r0, #1\n"
"	bne 1b\n"
"	bx lr\n"
"syscall: \n"
"	bkpt #0xAB\n"
"	bx lr\n"
);

extern void usleep(int i);
extern int syscall(int c, const void *p);

#define MODE_RB 1
#define MODE_WB 5
int open(const char *s, int mode) {
    uint32_t args[3] = {(uint32_t) s, mode, strlen(s)};
    return syscall(0x01, args);
}

int read(int fd, void *s, int n) {
    uint32_t args[3] = {fd, (uint32_t) s, n};
    return syscall(0x06, args);
}

int write(int fd, void *s, int n) {
    uint32_t args[3] = {fd, (uint32_t) s, n};
    return syscall(0x05, args);
}

int close(int fd) {
    return syscall(0x02, (void *)fd);
}

const unsigned char *puts(const unsigned char *_s) {
    syscall(0x04 /* SYS_WRITE0 */, _s);
}

void printf(const char *ifmt, ...) {
    char buf[128];
    va_list ap;
    int n;
    
    va_start(ap, ifmt);
    n = vsfmt(buf, 128, ifmt, ap);
    va_end(ap);
    
    puts(buf);
}

#define JEDEC_READ 0x03
#define JEDEC_RDSR 0x05
#define JEDEC_RDFLGSR 0x70
#define JEDEC_CLRFLGSR 0x50
#define JEDEC_PAGE_PROGRAM 0x02
#define JEDEC_IDCODE 0x9F
#define JEDEC_DUMMY 0xA9
#define JEDEC_WAKE 0xAB
#define JEDEC_SLEEP 0xB9
#define JEDEC_WRITE_ENABLE 0x06
#define JEDEC_SECTOR_ERASE 0xD8

#define JEDEC_RDSR_BUSY 0x01

#define JEDEC_FLGSR_PROGBUSY 0x80
#define JEDEC_FLGSR_ERASEFAIL 0x10
#define JEDEC_FLGSR_PROGFAIL 0x08

#define JEDEC_IDCODE_MICRON_N25Q032A11 0x20BB16 /* bianca / qemu / ev2_5 */

#define PAGESIZE 256

static uint8_t _hw_flash_txrx(uint8_t c) {
    while (!(SPI1->SR & SPI_SR_TXE))
        ;
    SPI1->DR = c;
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;
    return SPI1->DR;
}

static void _hw_flash_enable(int i) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIO_WriteBit(GPIOA, 1 << 4, !i);
    usleep(1);
}

static void _hw_flash_wfidle() {
    _hw_flash_enable(1);
    uint8_t sr;
    do {
        _hw_flash_txrx(JEDEC_RDSR);
        sr = _hw_flash_txrx(JEDEC_DUMMY);
    } while (sr & JEDEC_RDSR_BUSY);
    _hw_flash_enable(0);
}

static void _hw_flash_write_enable() {
    _hw_flash_enable(1);
    _hw_flash_txrx(JEDEC_WRITE_ENABLE);
    _hw_flash_enable(0);
}


void hw_flash_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    
    /* Set up the pins. */
    GPIO_WriteBit(GPIOA, 1 << 4, 0); /* nCS */
    GPIO_PinAFConfig(GPIOA, 5, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, 6, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, 7, GPIO_AF_SPI1);
    
    GPIO_InitTypeDef gpioinit;
    
    gpioinit.GPIO_Pin = (1 << 7) | (1 << 6);
    gpioinit.GPIO_Mode = GPIO_Mode_AF;
    gpioinit.GPIO_Speed = GPIO_Speed_50MHz;
    gpioinit.GPIO_OType = GPIO_OType_PP;
    gpioinit.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpioinit);

    gpioinit.GPIO_Pin = (1 << 5);
    gpioinit.GPIO_Mode = GPIO_Mode_AF;
    gpioinit.GPIO_Speed = GPIO_Speed_50MHz;
    gpioinit.GPIO_OType = GPIO_OType_PP;
    gpioinit.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOA, &gpioinit);

    gpioinit.GPIO_Pin = (1 << 4);
    gpioinit.GPIO_Mode = GPIO_Mode_OUT;
    gpioinit.GPIO_Speed = GPIO_Speed_50MHz;
    gpioinit.GPIO_OType = GPIO_OType_PP;
    gpioinit.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpioinit);
    
    /* Set up the SPI controller, SPI1. */
    SPI_InitTypeDef spiinit;
    
    RCC->APB2ENR |= RCC_APB2Periph_SPI1;

    SPI_I2S_DeInit(SPI1);
    
    spiinit.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spiinit.SPI_Mode = SPI_Mode_Master;
    spiinit.SPI_DataSize = SPI_DataSize_8b;
    spiinit.SPI_CPOL = SPI_CPOL_Low;
    spiinit.SPI_CPHA = SPI_CPHA_1Edge;
    spiinit.SPI_NSS = SPI_NSS_Soft;
    spiinit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    spiinit.SPI_FirstBit = SPI_FirstBit_MSB;
    spiinit.SPI_CRCPolynomial = 7 /* Um. */;
    SPI_Init(SPI1, &spiinit);
    SPI_Cmd(SPI1, ENABLE);
    
    /* In theory, SPI is up.  Now let's see if we can talk to the part. */
    _hw_flash_enable(1);
    _hw_flash_txrx(JEDEC_WAKE);
    _hw_flash_enable(0);
    usleep(100);
    
    _hw_flash_wfidle();
     
    uint32_t part_id = 0;
    
    _hw_flash_enable(1);
    _hw_flash_txrx(JEDEC_IDCODE);
    part_id |= _hw_flash_txrx(JEDEC_DUMMY) << 16;
    part_id |= _hw_flash_txrx(JEDEC_DUMMY) << 8;
    part_id |= _hw_flash_txrx(JEDEC_DUMMY) << 0;
    _hw_flash_enable(0);
    
    printf("tintin flash: JEDEC ID %08lx\n", part_id);
}


void hw_flash_read_bytes(uint32_t addr, uint8_t *buf, int len) {
    _hw_flash_wfidle();
    
    _hw_flash_enable(1);
    _hw_flash_txrx(JEDEC_READ);
    _hw_flash_txrx((addr >> 16) & 0xFF);
    _hw_flash_txrx((addr >>  8) & 0xFF);
    _hw_flash_txrx((addr >>  0) & 0xFF);
    
    /* XXX: could use DMA, conceivably */
    for (int i = 0; i < len; i++)
        buf[i] = _hw_flash_txrx(JEDEC_DUMMY);
    
    _hw_flash_enable(0);
}

int hw_flash_erase(uint32_t addr) {
    _hw_flash_wfidle();

    _hw_flash_enable(1);
    _hw_flash_txrx(JEDEC_WRITE_ENABLE);
    _hw_flash_enable(0);

    _hw_flash_enable(1);
    _hw_flash_txrx(JEDEC_CLRFLGSR);
    _hw_flash_enable(0);

    _hw_flash_enable(1);
    _hw_flash_txrx(JEDEC_SECTOR_ERASE);
    _hw_flash_txrx((addr >> 16) & 0xFF);
    _hw_flash_txrx((addr >>  8) & 0xFF);
    _hw_flash_txrx((addr >>  0) & 0xFF);
    
    _hw_flash_enable(0);
    
    _hw_flash_enable(1);
    uint8_t sr;
    do {
        _hw_flash_txrx(JEDEC_RDFLGSR);
        sr = _hw_flash_txrx(JEDEC_DUMMY);
    } while (sr & JEDEC_FLGSR_PROGBUSY);
    _hw_flash_enable(0);
    
    if (sr & JEDEC_FLGSR_ERASEFAIL) {
        printf("address erase %06x failed!\n", addr);
        return 1;
    }
    
    return 0;
}

int hw_flash_prog_page(uint32_t addr, uint8_t *p) {
    _hw_flash_wfidle();

    _hw_flash_enable(1);
    _hw_flash_txrx(JEDEC_WRITE_ENABLE);
    _hw_flash_enable(0);

    _hw_flash_enable(1);
    _hw_flash_txrx(JEDEC_CLRFLGSR);
    _hw_flash_enable(0);

    _hw_flash_enable(1);
    _hw_flash_txrx(JEDEC_PAGE_PROGRAM);
    _hw_flash_txrx((addr >> 16) & 0xFF);
    _hw_flash_txrx((addr >>  8) & 0xFF);
    _hw_flash_txrx((addr >>  0) & 0xFF);
    
    for (int i = 0; i < PAGESIZE; i++)
        _hw_flash_txrx(p[i]);
    
    _hw_flash_enable(0);
    
    _hw_flash_enable(1);
    uint8_t sr;
    do {
        _hw_flash_txrx(JEDEC_RDFLGSR);
        sr = _hw_flash_txrx(JEDEC_DUMMY);
    } while (sr & JEDEC_FLGSR_PROGBUSY);
    _hw_flash_enable(0);
    
    if (sr & JEDEC_FLGSR_PROGFAIL) {
        printf("address program %06x failed!\n", addr);
        return 1;
    }
    
    return 0;
}

#define BUFSIZ 16384
#define FLASHSIZ 4*1024*1024
char flashbuf[BUFSIZ];

/* for GDB... */
void *malloc(int sz) {
    printf("XXX: cheesy malloc %d\n", sz);
    return flashbuf;
}

void flashrd(char *filename) {
    printf("Dumping %d bytes of SPI flash...\n", FLASHSIZ);

    int fd = open(filename, MODE_WB);
    for (uint32_t i = 0; i < FLASHSIZ; i += BUFSIZ) {
        if ((i % (512 * 1024)) == 0) {
            printf("...%d / %d...\n", i, FLASHSIZ);
        }
        hw_flash_read_bytes(i, flashbuf, BUFSIZ);
        write(fd, flashbuf, BUFSIZ);
    }
    close(fd);

    puts("Done!\n");
}

void flashwr(char *filename) {
    hw_flash_init();
    
    int fd = open(filename, MODE_RB);
    if (fd < 0) {
        printf("failed to open %s\n", filename);
        return;
    }
    for (uint32_t i = 0; i < FLASHSIZ; i += BUFSIZ) {
        read(fd, flashbuf, BUFSIZ);
        if ((i % (64 * 1024)) == 0) {
            printf("Erasing %06x...\n", i);
            if (hw_flash_erase(i))
                goto done;
        }
        for (uint32_t j = 0; j < BUFSIZ; j += PAGESIZE) {
            if (hw_flash_prog_page(i + j, flashbuf + j))
                goto done;
        }
    }
done:
    close(fd);

    puts("Done!\n");        
}

int main() {
    hw_flash_init();
    printf("Flash is initialized ... have fun\n");
    while(1)
        ;
}
