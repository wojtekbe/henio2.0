#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "fast_hsv2rgb.h"

/*
  ,---------,
  |         |    MOSI    ,------,
  | D11,PB2 o------------o DATA |
  |         |     ,----, '------'
  | D2, PD2 o-----o A  | LEDring
  |         |     |    |
  | D3, PD3 o-----o B  |
  |         |     |    |
  | D4, PD4 o-----o SW |
  |         |     '----'
  '---------'    encoder
   atmega328
*/

#define LEDRING_COUNT 24

#define FX_UNICORN_V_ON  128
#define FX_UNICORN_V_OFF   4
#define FX_UNICORN_S     250
#define FX_UNICORN_V_FADEOUT_STEP 2

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

typedef struct _pix_hsv {
    uint16_t H;
    uint8_t  S;
    uint8_t  V;
} pix_hsv;

pix_hsv frame_hsv[LEDRING_COUNT];

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

    /* fx init */
    d->fx_last_n = 0xFF;
    d->fx_hsv_count = 0;
    for (int i = 0; i<(d->ledcount); i++) {
        frame_hsv[i].H = i*HSV_HUE_STEPS/23;
        frame_hsv[i].S = FX_UNICORN_S;
        frame_hsv[i].V = FX_UNICORN_V_OFF;
    }

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

struct enc_data {
    uint8_t is; // current status (in gray code)
    uint8_t was; // last status
    int8_t count; // counter state
    uint8_t maxcount; // max counter val

    void (*on_change)(uint8_t*);
} enc;

void hsv2rgb(pix_hsv *Phsv, pix *P)
{
    return fast_hsv2rgb_8bit(Phsv->H, Phsv->S, Phsv->V,
                             &(P->R), &(P->G), &(P->B));
}

void ledring_fx_unicorn(struct ledring_data *d, void *a) //🦄
{
    pix P;
    uint16_t h;
    uint8_t v;

    /* fade out all 'old' pixels */
    for (int i = 0; i<(d->ledcount); i++) {
        if (frame_hsv[i].V > FX_UNICORN_V_OFF)
            frame_hsv[i].V -= FX_UNICORN_V_FADEOUT_STEP;
    }

    /* light the 'new' pixel */
    frame_hsv[ d->fx_hsv_count ].V = FX_UNICORN_V_ON;

    /* render frame */
    cli();
    for (int i = 0; i<(d->ledcount); i++) {
        hsv2rgb(&frame_hsv[i], &P);
        ledring_set_pix(&P);
    }
    sei();
    ledring_show();
}

#define enc_rd() ((PIND>>2) & 0x3)

void enc_init(struct enc_data *d, uint8_t range, void (*f)(uint8_t*))
{
    DDRD &= ~(_BV(PIND2) | _BV(PIND3));
    PORTD |=  _BV(PIND2) | _BV(PIND3);

    d->was = enc_rd();
    d->is = d->was;
    d->count = 0;
    d->maxcount = range;

    d->on_change = f;
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
            if (d->count < 0) d->count = (d->maxcount-1);
            d->on_change(&(d->count));
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
            if (d->count >= d->maxcount) d->count = 0;
            d->on_change(&(d->count));
        }

        d->was = d->is;
    }
}

void on_enc_change(uint8_t *cnt)
{
    leds.fx_hsv_count = *cnt;
}

int main(void)
{
    ledring_init(&leds, LEDRING_COUNT, ledring_fx_unicorn); // 🚦
    enc_init(&enc, LEDRING_COUNT, on_enc_change); // 🌀

    while (1) {
        enc_update(&enc);
        _delay_ms(2);

        leds.redraw(&leds, (void*)&(enc.count));
    }
}
