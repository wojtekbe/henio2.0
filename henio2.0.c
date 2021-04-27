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
    uint8_t fx_style_idx;
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
    d->fx_style_idx = 0;

    ledring_blank(d);
}

struct style {pix bg; pix fg;};
struct style themes[] = {
    { {0, 0, 1}, {16, 0, 92} },
    { {1, 0, 0}, {92, 0, 16} },
    { {0, 1, 0}, {92, 0, 16} }
};
#define NUM_STYLES (sizeof(themes) / sizeof(struct style))

void ledring_fx_single(struct ledring_data *d, int n, uint8_t next)
{
    //if (n == d->fx_last_n) return;
    if (next) d->fx_style_idx = (d->fx_style_idx+1)%NUM_STYLES;

    cli();
    for (int i = 0; i<(d->ledcount); i++) {
        if ( i == n )
            ledring_set_pix(&themes[d->fx_style_idx].fg);
        else
            ledring_set_pix(&themes[d->fx_style_idx].bg);
    }
    sei();
    ledring_show();

    d->fx_last_n = n;
}

struct enc_data {
    uint8_t is; // current status (in grey code)
    uint8_t was; // last status
    uint8_t btn; // button state
    int8_t count; // counter state
    uint8_t count_range; // max counter val
} enc;

#define enc_rd()     ((PIND>>2) & 0x3)
#define enc_rd_btn() (~(PIND>>4) & 0x1)

void enc_init(struct enc_data *d, uint8_t range)
{
    DDRD &= ~(_BV(PIND2) | _BV(PIND3) | _BV(PIND4));
    PORTD |=  _BV(PIND2) | _BV(PIND3) | _BV(PIND4);

    d->was = enc_rd();
    d->is = d->was;
    d->btn = 0;
    d->count = 0;
    d->count_range = range;
}

void enc_update(struct enc_data *d)
{
    d->is = enc_rd(); // A/B logic is inverted

    // rotary position
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

    // button
    if (d->btn==0 && enc_rd_btn()==1) { // btn down
        d->btn = 1;
    } else
    if ((d->btn==1 && enc_rd_btn()==0) ||
        (d->btn == enc_rd_btn())) { // btn up
        d->btn = 0;
    }
}

int main(void)
{
    ledring_init(&leds, LEDRING_COUNT);
    enc_init(&enc, ENCODER_RANGE);

    while (1) {
        enc_update(&enc);

        ledring_fx_single(&leds, enc.count, enc.btn);
        _delay_ms(2);
    }
}
