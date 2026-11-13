#include <stdint.h>

#define BAUD      115200u
#define UART_CLK  32000000u  /* ASSUMED bus clock. If text is garbled, try 16000000u */
#define UART_OVS  16u

#define GPIOB_BASE 0x400A2000u
#define IOMUX_BASE 0x40428000u
#define UART0_BASE 0x40108000u

#define REG(a) (*(volatile uint32_t *)(a))

/* LED on PB14: same registers as your working blink */
#define GPIOB_PWREN REG(GPIOB_BASE + 0x0800u)
#define GPIOB_DOE   REG(GPIOB_BASE + 0x12C0u)
#define GPIOB_TGL   REG(GPIOB_BASE + 0x12B0u)
/* PINCMn lives at array index n-1 (header: PINCM1 = 0, PINCM35 = 34) */
#define PINCM(n)    REG(IOMUX_BASE + 0x4u + 4u * ((n) - 1u))

/* UART0 registers: offsets checked against hw_uart.h */
#define UART0_PWREN   REG(UART0_BASE + 0x0800u)
#define UART0_RSTCTL  REG(UART0_BASE + 0x0804u)
#define UART0_CLKDIV  REG(UART0_BASE + 0x1000u)
#define UART0_CLKSEL  REG(UART0_BASE + 0x1008u)
#define UART0_PDBGCTL REG(UART0_BASE + 0x1018u)
#define UART0_CTL0    REG(UART0_BASE + 0x1100u)
#define UART0_LCRH    REG(UART0_BASE + 0x1104u)
#define UART0_STAT    REG(UART0_BASE + 0x1108u)
#define UART0_IBRD    REG(UART0_BASE + 0x1110u)
#define UART0_FBRD    REG(UART0_BASE + 0x1114u)
#define UART0_TXDATA  REG(UART0_BASE + 0x1120u)

#define PWREN_KEY   0x26000000u
#define RSTCTL_KEY  0xB1000000u
#define PINCM_PC    0x80u            /* peripheral connected */
#define PINCM_INENA (1u << 18)       /* input enable (needed on RX) */
#define STAT_TXFF   (1u << 7)        /* TX FIFO full */

extern uint32_t _estack;
int main(void);
void reset_handler(void);

static void delay(uint32_t n) { for (volatile uint32_t i = 0; i < n; i++) {} }
static void led_toggle(void)  { GPIOB_TGL = (1u << 14); }

static void led_init(void) {
    GPIOB_PWREN = PWREN_KEY | 1u;
    delay(100);
    PINCM(35) = 0x01u | PINCM_PC;    /* PB14 = PINCM35, function 1 (GPIO) */
    GPIOB_DOE |= (1u << 14);
}

/* Blink n times, then pause. Counting blinks shows how far setup got. */
static void stage(uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        led_toggle(); delay(150000);
        led_toggle(); delay(150000);
    }
    delay(900000);
}

/* Any fault lands here: LED blinks FAST forever. */
void default_handler(void) {
    for (;;) { led_toggle(); delay(30000); }
}

__attribute__((section(".vectors"), used))
void (* const vectors[16])(void) = {
    (void (*)(void))&_estack, reset_handler, default_handler, default_handler,
    0, 0, 0, 0, 0, 0, 0,
    default_handler, 0, 0, default_handler, default_handler
};

static void uart_init(void) {
    /* Reset first, then power on (power-on resets to off, so order matters) */
    UART0_RSTCTL = RSTCTL_KEY | 1u;
    UART0_RSTCTL = RSTCTL_KEY;
    UART0_PWREN  = PWREN_KEY | 1u;
    delay(1000);
    stage(1);                                  /* 1 blink: UART powered */

    PINCM(25) = 2u | PINCM_PC;                 /* PA10 = UART0_TX */
    PINCM(26) = 2u | PINCM_PC | PINCM_INENA;   /* PA11 = UART0_RX */
    stage(2);                                  /* 2 blinks: pins routed */

    UART0_CLKSEL  = (1u << 3);                 /* bus clock */
    UART0_CLKDIV  = 0;                         /* divide by 1 */
    UART0_PDBGCTL = 1u;                        /* keep running while halted */
    stage(3);                                  /* 3 blinks: clock set */

    UART0_CTL0 = 0;                            /* off while configuring */
    uint32_t div  = UART_OVS * BAUD;
    uint32_t ibrd = UART_CLK / div;
    uint32_t rem  = UART_CLK % div;
    UART0_IBRD = ibrd;
    UART0_FBRD = ((rem * 64u) + (div / 2u)) / div;
    UART0_LCRH = (3u << 4);                    /* 8 data bits, no parity, 1 stop */
    UART0_CTL0 = (1u << 0) | (1u << 3) | (1u << 4);  /* enable + RX + TX */
    stage(4);                                  /* 4 blinks: UART enabled */
}

static void putchar_uart(char c) {
    while (UART0_STAT & STAT_TXFF) {}          /* wait while TX FIFO full */
    UART0_TXDATA = (uint32_t)(uint8_t)c;
}

static void puts_uart(const char *s) { while (*s) putchar_uart(*s++); }

int main(void) {
    led_init();
    uart_init();
    for (;;) {
        led_toggle();                          /* steady blink = still printing */
        puts_uart("Hello, world!\r\n");
        delay(2000000);
    }
}

void reset_handler(void) { main(); for (;;) {} }
