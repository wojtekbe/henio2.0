#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define LEDRING_COUNT 24
#define ENCODER_RANGE LEDRING_COUNT

#define spi_transfer(data) \
    do {SPDR=data; while(!(SPSR&(1<<SPIF)));} while(0)

struct ledring_data {
    uint8_t ledcount;
    uint8_t fx_last_n;
} leds;

typedef struct _pix {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} pix;

void ledring_send_byte(uint8_t b) {
    for (uint8_t i = 0; i<8; i++) {
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

void ledring_show(void)
{
    _delay_us(10);
}

void ledring_blank(struct ledring_data *d)
{
    pix black = {0, 0, 0};
    _delay_us(20);
    for (int i = 0; i<(d->ledcount); i++) {
        ledring_set_pix(&black);
    }
    _delay_us(50);
}

void ledring_init(struct ledring_data *d, uint8_t count)
{
    DDRB |= _BV(PINB2) | _BV(PINB3) | _BV(PINB5);
    SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
    SPSR |= _BV(SPI2X);
    PORTB |= _BV(PINB2);

    d->ledcount = count;
    d->fx_last_n = 0xFF;

    ledring_blank(d);
}

void ledring_fx_single(struct ledring_data *d, int n, pix *P_on)
{
    if (n == d->fx_last_n) return;
    pix P_off = { 0, 0, 1 };

    cli();
    for (int i = 0; i<(d->ledcount); i++) {
        if ( i == n )
            ledring_set_pix(P_on);
        else
            ledring_set_pix(&P_off);
    }
    sei();
    ledring_show();

    d->fx_last_n = n;
}

struct enc_data {
    uint8_t is; // current status (in grey code)
    uint8_t was; // last status
    int8_t count; // counter state
    uint8_t count_range; // max counter val
} enc;

#define enc_rd() ((PIND>>2) & 0x3)

void enc_init(struct enc_data *d, uint8_t range)
{
    DDRD &= ~(_BV(PIND2) | _BV(PIND3));
    PORTD |=  _BV(PIND2) | _BV(PIND3);

    d->was = enc_rd();
    d->is = d->was;
    d->count = 0;
    d->count_range = range;
}

void enc_update(struct enc_data *d)
{
    d->is = enc_rd(); // A/B logic is inverted

    if (d->is != d->was) {
        if ((d->was==3 && d->is==2) ||
            (d->was==1 && d->is==3)) {
//            __    ___
//         A    |  |
//              '--'
//            ___    __
//         B     |  |
//               '--'
//      CODE  *3200133*
//              ^   ^
            d->count--;
            if (d->count < 0) d->count = (d->count_range-1);
        } else
        if ((d->was==3 && d->is==1) ||
            (d->was==2 && d->is==3)) {
//            ___    __
//         A     |  |
//               '--'
//            __    ___
//         B    |  |
//              '--'
//      CODE  *3100233*
//              ^   ^
            d->count++;
            if (d->count >= d->count_range) d->count = 0;
        }

        d->was = d->is;
    }
}

int main(void)
{
    ledring_init(&leds, LEDRING_COUNT);
    enc_init(&enc, ENCODER_RANGE);

    pix P_on = { 16, 0, 92 };
    while (1) {
        ledring_fx_single(&leds, enc.count, &P_on);
        _delay_ms(2);

        enc_update(&enc);

    }
}
