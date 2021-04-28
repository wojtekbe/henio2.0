#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "fast_hsv2rgb.h"

#define LEDRING_COUNT 24
#define ENCODER_RANGE LEDRING_COUNT

#define spi_transfer(data) \
    do {SPDR=data; while(!(SPSR&(1<<SPIF)));} while(0)

struct ledring_data {
    uint8_t ledcount;
    void (*redraw)(struct ledring_data*, void*);

    uint8_t fx_last_n;
    uint8_t fx_hsv_count;
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

void ledring_init(struct ledring_data *d, uint8_t count, void (*f)(struct ledring_data*, void*))
{
    DDRB |= _BV(PINB2) | _BV(PINB3) | _BV(PINB5);
    SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
    SPSR |= _BV(SPI2X);
    PORTB |= _BV(PINB2);

    d->ledcount = count;
    d->redraw = f;

    d->fx_last_n = 0xFF;
    d->fx_hsv_count = 0;

    ledring_blank(d);
}

void ledring_fx_single(struct ledring_data *d, void* p)
{
    int8_t n = *((int8_t*)p);
    if (n == d->fx_last_n) return;
    pix P_off = { 0, 0, 1 };
    pix P_on  = { 5, 5, 5 };

    cli();
    for (int i = 0; i<(d->ledcount); i++) {
        if ( i == n )
            ledring_set_pix(&P_on);
        else
            ledring_set_pix(&P_off);
    }
    sei();
    ledring_show();

    d->fx_last_n = n;
}

// nice & predefined colors
#define GREEN  {0,   79,  5}
#define YELLOW {134, 136, 0}
#define ORANGE {136, 101, 0}
#define RED    {206, 34,  11}
#define PINK   {136, 0,   122}
#define VIOLET {86,  0,   136}
#define BLUE   {2,   0,   136}
#define LBLUE  {0,   114, 136}


void ledring_fx_frame(struct ledring_data *d, void* f)
{
    pix *frame = (pix *)f;

    cli();
    for (int i = 0; i<(d->ledcount); i++) {
        ledring_set_pix(&frame[i]);
    }
    sei();
    ledring_show();
}

struct enc_data {
    uint8_t is; // current status (in gray code)
    uint8_t was; // last status
    int8_t count; // counter state
    uint8_t count_range; // max counter val
} enc;

void hsv2rgb(uint16_t h, uint8_t s, uint8_t v, pix *P)
{
    return fast_hsv2rgb_8bit(h, s, v, &(P->R), &(P->G), &(P->B));
}

#define FX_UNICORN_V_ON  127
#define FX_UNICORN_V_OFF   4
#define FX_UNICORN_S     250
void ledring_fx_unicorn(struct ledring_data *d, void *a) //🦄
{
    pix P;
    uint16_t h;
    uint8_t v;

    cli();
    for (int i = 0; i<(d->ledcount); i++) {
        h = i*HSV_HUE_STEPS/23;
        if (i == d->fx_hsv_count)
            v = FX_UNICORN_V_ON;
        else
            v = FX_UNICORN_V_OFF;

        hsv2rgb(h, FX_UNICORN_S, v, &P);
        ledring_set_pix(&P);
    }
    sei();
    ledring_show();
}

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

pix frame[24] = {
    GREEN,  YELLOW, ORANGE, RED,    LBLUE, BLUE,  VIOLET, PINK,
    GREEN,  RED,    LBLUE,  ORANGE, PINK,  LBLUE, BLUE,   VIOLET,
    ORANGE, PINK,   ORANGE, VIOLET, RED,   RED,   VIOLET, PINK
};

int main(void)
{
    ledring_init(&leds, LEDRING_COUNT, ledring_fx_unicorn); // 🚦
    enc_init(&enc, ENCODER_RANGE); // 🌀

    while (1) {
        enc_update(&enc);
        leds.fx_hsv_count = enc.count;
        _delay_ms(2);

        //leds.redraw(&leds, (void*)&(enc.count));
        leds.redraw(&leds, 0);
    }
}
