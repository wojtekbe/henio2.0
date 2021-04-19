#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define SPI_DDR  DDRB
#define SPI_CS   PINB2 //-> D10
#define SPI_MOSI PINB3 //-> D11
#define SPI_SCK  PINB5 //-> D13

#define LEDRING_COUNT 24

#define spi_transfer(data) \
    do {SPDR=data; while(!(SPSR&(1<<SPIF)));} while(0)

typedef struct _pix {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} pix;

void ledring_send_byte(uint8_t b) {
    for (int i = 0; i<8; i++) {
        if (b & 0x80)
            spi_transfer(0b11111100);
        else
            spi_transfer(0b11100000);

        b <<= 1;
    }
}

void ledring_set_pix(pix* px)
{
    ledring_send_byte(px->G);
    ledring_send_byte(px->R);
    ledring_send_byte(px->B);
}

void ledring_show()
{
    _delay_us(10);
}

void ledring_init(void)
{
    DDRB |= _BV(PINB2) | _BV(PINB3) | _BV(PINB5);
    SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
    SPSR |= _BV(SPI2X);
    PORTB |= _BV(PINB2);

    pix black = {0, 0, 0};
    _delay_us(20);
    for (int i = 0; i<LEDRING_COUNT; i++) {
        ledring_set_pix(&black);
    }
    _delay_us(50);
}

void ledring_fx_single(int n, pix *P_on)
{
    pix P_off = { 0, 0, 1 };
    n = n%LEDRING_COUNT;
    cli();
    for (int i = 0; i<LEDRING_COUNT; i++) {
        if ( i == n )
            ledring_set_pix(P_on);
        else
            ledring_set_pix(&P_off);
    }
    ledring_show();
    sei();
}

int main(void)
{
    ledring_init();

    pix P_on = { 16, 0, 92 };
    int l = 0;
    while (1) {
        ledring_fx_single(l++, &P_on);
        _delay_ms(10);
    }

}
