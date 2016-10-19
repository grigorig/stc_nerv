#include <stdint.h>

static uint32_t x = 1, y = 2, z = 3, w = 5;

uint32_t xorshift128(void)
{
    uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}
