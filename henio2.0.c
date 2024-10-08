#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stddef.h>
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
#define FX_UNICORN_V_FADEOUT_STEP 1

#define spi_transfer(data) \
    do {SPDR=data; while(!(SPSR&(1<<SPIF)));} while(0)

struct ledring_data {
    uint8_t ledcount;
    void (*redraw)(struct ledring_data*);

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

void ledring_init(struct ledring_data *d, uint8_t count, void (*redraw_f)(struct ledring_data*))
{
    DDRB |= _BV(PINB2) | _BV(PINB3) | _BV(PINB5);
    SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
    SPSR |= _BV(SPI2X);
    PORTB |= _BV(PINB2);

    d->ledcount = count;
    d->redraw = redraw_f;

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

void ledring_fx_single(struct ledring_data *d)
{
    int8_t n = d->fx_hsv_count;
    pix P_off = { 8, 0, 0 };
    pix P_on  = { 128, 0, 0 };

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
    void (*on_rotate)(bool up);
    int count;

    bool btn_now; // button state now
    bool btn_last; // button state last
    void (*on_btn)(bool pressed);
} enc;

void hsv2rgb(pix_hsv *Phsv, pix *P)
{
    return fast_hsv2rgb_8bit(Phsv->H, Phsv->S, Phsv->V,
                             &(P->R), &(P->G), &(P->B));
}

void ledring_fx_unicorn(struct ledring_data *d) //🦄
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

void ledring_fx_single_selected(struct ledring_data *d)
{
    pix P;
    pix_hsv Ph;
    int8_t n = d->fx_hsv_count;

    Ph = frame_hsv[n];
    Ph.V = FX_UNICORN_V_ON;

    cli();
    for (int i = 0; i<(d->ledcount); i++) {
        hsv2rgb(&Ph, &P);
        ledring_set_pix(&P);
    }
    sei();
    ledring_show();
}

void ledring_change_fx(struct ledring_data *d, void (*new_fx)(struct ledring_data*))
{
    d->redraw = new_fx;
}

#define enc_rd_rotation() ((PIND>>2) & 0x3)
#define enc_rd_button() (((PIND>>4) & 0x1) ? false : true)

void enc_init(struct enc_data *d, void (*rot_f)(bool), void (*btn_f)(bool))
{
    DDRD &= ~(_BV(PIND2) | _BV(PIND3) | _BV(PIND4));
    PORTD |=  _BV(PIND2) | _BV(PIND3) | _BV(PIND4);

    d->was = enc_rd_rotation();
    d->is = d->was;

    d->on_rotate = rot_f;
    d->count = 0;

    d->btn_now = false;
    d->btn_last = false;

    d->on_btn = btn_f;
}

void enc_update(struct enc_data *d)
{
    // update position if changed
    d->is = enc_rd_rotation(); // A/B logic is inverted

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
            if (d->count % 2) { // state changes twice per encoder "click"
                d->on_rotate(false); // rotate down
            }
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
            if (d->count % 2) { // state changes twice per encoder "click"
                d->on_rotate(true); // rotate up
            }
        }

        d->was = d->is;
    }

    // update button status if changed
    // and call user callback
    d->btn_now = enc_rd_button();
    if (d->btn_now != d->btn_last) {
        d->on_btn(d->btn_now);
        d->btn_last = d->btn_now;
    }
}

void on_enc_change(bool up)
{
    static int8_t cnt;

    cnt = up ? cnt+1 : cnt-1;
    if (cnt >= leds.ledcount) cnt = 0;
    if (cnt < 0) cnt = (leds.ledcount - 1);

    leds.fx_hsv_count = cnt;
}

void on_enc_button(bool down)
{
    if (down) {
        ledring_change_fx(&leds, ledring_fx_single_selected);
    } else {
        ledring_change_fx(&leds, ledring_fx_unicorn);
    }
}

int main(void)
{
    ledring_init(&leds, LEDRING_COUNT, ledring_fx_unicorn); // 🚦
    enc_init(&enc, on_enc_change, on_enc_button); // 🌀

    while (1) {
        enc_update(&enc);
        _delay_ms(2);

        leds.redraw(&leds);
    }
}
