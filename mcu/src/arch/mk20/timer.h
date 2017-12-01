#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

typedef uint32_t timer_ms_t;

extern volatile timer_ms_t systick_millis_count;

extern void timer_delay(uint32_t ms);

static inline timer_ms_t timer_millis(void)
{
    volatile timer_ms_t ret = systick_millis_count;
    return ret;
}

static inline int timer_millis_timed_out(timer_ms_t start, uint16_t timeout)
{
    return (timer_millis() - start) > timeout;
}

/* from Teensyduino: core_pins.h */
static inline void timer_delay_us(uint32_t) __attribute__((always_inline, unused));
static inline void timer_delay_us(uint32_t usec)
{
#if F_CPU == 240000000
    uint32_t n = usec * 80;
#elif F_CPU == 216000000
    uint32_t n = usec * 72;
#elif F_CPU == 192000000
    uint32_t n = usec * 64;
#elif F_CPU == 180000000
    uint32_t n = usec * 60;
#elif F_CPU == 168000000
    uint32_t n = usec * 56;
#elif F_CPU == 144000000
    uint32_t n = usec * 48;
#elif F_CPU == 120000000
    uint32_t n = usec * 40;
#elif F_CPU == 96000000
    uint32_t n = usec << 5;
#elif F_CPU == 72000000
    uint32_t n = usec * 24;
#elif F_CPU == 48000000
    uint32_t n = usec << 4;
#elif F_CPU == 24000000
    uint32_t n = usec << 3;
#elif F_CPU == 16000000
    uint32_t n = usec << 2;
#elif F_CPU == 8000000
    uint32_t n = usec << 1;
#elif F_CPU == 4000000
    uint32_t n = usec;
#elif F_CPU == 2000000
    uint32_t n = usec >> 1;
#endif
    // changed because a delay of 1 micro Sec @ 2MHz will be 0
    if (n == 0) return;
    __asm__ volatile(
        "L_%=_timer_delay_us:"       "\n\t"
#if F_CPU < 24000000
        "nop"                   "\n\t"
#endif
        "subs   %0, #1"             "\n\t"
        "bne    L_%=_timer_delay_us"     "\n"
        : "+r" (n) :
    );
}

#endif