#include <stdint.h>

#include "xorshift.h"
#include "stc15w.h"
#include "utils.h"

#define TFREQ(x) (65536 - (FOSC / x / 2))
#define TICK_RELOAD (65536 - (FOSC / 100))
#define nop4() nop(); nop(); nop(); nop()

#define DEBUG_SLEEP

#define WAKEUP_10S 20491
#define WAKEUP_15S 30737

volatile unsigned tick;
volatile __bit tick_overflow;

void beep_isr(void) __interrupt TF0_VECTOR __using 1
{
    P1_1 ^= 1;
}

void tick_isr(void) __interrupt 12 __using 2
{
    tick++;
    if (tick == 0)
        tick_overflow = 1;
}

void set_timer_osc(unsigned freq)
{
    if (freq == 0) {
        TR0 = 0;
        ET0 = 0;
        return;
    }

    AUXR |= T0X12;
    TMOD = 0;
    TL0 = freq & 0xff;
    TH0 = freq >> 8;
    TR0 = 1;
    ET0 = 1;
    PT0 = 1;
    P1M0 |= (1 << 1);
}

void init_tick(void)
{
    tick = 0;
    AUXR |= 0x14;
    T2L = TICK_RELOAD & 0xff;
    T2H = TICK_RELOAD >> 8;
    IE2 |= 0x04;
}

void tick_delay(unsigned delay)
{
    IE2 &= ~0x04;
    tick = 65536 - delay;
    tick_overflow = 0;
    IE2 |= 0x04;

    /* sleep while waiting for delay */
    while(!tick_overflow) {
        PCON |= 0x01;
        nop4();
    }
}

void power_down_delay(unsigned delay)
{
    WKTCL = delay & 0xff;
    WKTCH = (delay >> 8) | 0x80;
    PCON |= 0x02;
    nop4(); nop4();
}

void power_down_delay_long(unsigned minutes)
{
    unsigned delay_waits = 4 * minutes;
    int i;

    for (i = 0; i < delay_waits; i++)
        power_down_delay(WAKEUP_15S);
}

void do_the_beeps(void)
{
    uint32_t rand = xorshift128() % 5;

    switch (rand) {
    case 0:
        /* Just a long beep */
        set_timer_osc(TFREQ(4000));
        tick_delay(150);
        set_timer_osc(0);
        break;
    case 1:
        /* Double beep */
        set_timer_osc(TFREQ(4000));
        tick_delay(75);
        set_timer_osc(0);
        tick_delay(10);
        set_timer_osc(TFREQ(4000));
        tick_delay(75);
        set_timer_osc(0);
        break;
    case 2: {
            /* Alarm? */
            int i;
            for (i = 0; i < 2; i++) {
                set_timer_osc(TFREQ(4000));
                tick_delay(50);
                set_timer_osc(TFREQ(3520));
                tick_delay(50);
            }
            set_timer_osc(0);
        }
        break;
    case 3: {
            /* Random */
            int i;
            for (i = 0; i < 120; i++) {
                //set_timer_osc(32768 + (xorshift128() & 0x7fff));
                set_timer_osc(64000 + (xorshift128() & 0xff));
                tick_delay(1);
            }
            set_timer_osc(0);
        }
        break;
    case 4:
        /* Three beeps */
        set_timer_osc(TFREQ(4000));
        tick_delay(40);
        set_timer_osc(0);
        tick_delay(10);
        set_timer_osc(TFREQ(4000));
        tick_delay(40);
        set_timer_osc(0);
        tick_delay(10);
        set_timer_osc(TFREQ(4000));
        tick_delay(40);
        set_timer_osc(0);
        break;
    }
}

int main(void)
{
    PCON2 |= 0x01;
    P1_0 = 0;
    P1M0 |= (1 << 0);

    init_tick();
    set_timer_osc(0);
    EA = 1;

#ifndef DEBUG_SLEEP
    power_down_delay_long(2);
#endif

    for (;;) {
        uint32_t rand = (xorshift128() % 56) + 5;
#ifdef DEBUG_SLEEP
        P1_0 = 1;
#endif
        do_the_beeps();
#ifdef DEBUG_SLEEP
        P1_0 = 0;
#endif
#ifdef DEBUG_SLEEP
        tick_delay(1000);
#else
        if (P1_2 == 0)
            power_down_delay_long(1);
        else
            power_down_delay_long(rand);
#endif
    }
}
