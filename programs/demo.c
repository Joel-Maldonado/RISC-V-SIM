#include <stdint.h>

static uint32_t mix(uint32_t a, uint32_t b) {
    uint32_t x = (a << 3) ^ (b + 5U);
    x = (x | 0x22U) - (a & 0x7U);
    if ((int32_t)x < 0) {
        x = (uint32_t)(-(int32_t)x);
    }
    return x;
}

int main(void) {
    volatile uint8_t data[4] = {4U, 6U, 8U, 10U};
    uint32_t acc = 0U;
    uint32_t i;

    for (i = 0U; i < 4U; ++i) {
        acc += mix((uint32_t)data[i], i + 1U);
    }

    if (acc > 100U) {
        acc -= 250U;
    } else {
        acc += 250U;
    }

    acc = (acc >> 1) + 12U;

    if ((acc & 0x2aU) == 0x2aU && (acc ^ 0U) == 42U) {
        return (int)acc;
    }

    return 1;
}
