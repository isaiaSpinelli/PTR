#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "io_utils.h"
#include "audio_player_board.h"

static const unsigned char hex_digits[10] = {
    0x40, // 0
    0xf9, // 1
    0x24, // 2
    0x30, // 3
    0x19, // 4
    0x12, // 5
    0x02, // 6
    0x78, // 7
    0x00, // 8
    0x10, // 9
};

void display_time(void *ioctrl,
                  uint8_t minutes, uint8_t seconds, uint8_t centisecs)
{
    *((uint32_t*)(ioctrl + 0x70)) = hex_digits[minutes/10];
    *((uint32_t*)(ioctrl + 0x60)) = hex_digits[minutes%10];

    *((uint32_t*)(ioctrl + 0x50)) = hex_digits[seconds/10];
    *((uint32_t*)(ioctrl + 0x40)) = hex_digits[seconds%10];

    *((uint32_t*)(ioctrl + 0x30)) = hex_digits[centisecs/10];
    *((uint32_t*)(ioctrl + 0x20)) = hex_digits[centisecs%10];
}

uint32_t keys(void *ioctrl)
{
    return *((uint32_t*)(ioctrl + 0x90));
}

uint32_t switches(void *ioctrl)
{
    return *((uint32_t*)(ioctrl + 0xa0));
}


void set_volume_leds(void *ioctrl, uint8_t volume)
{
    uint32_t val = 0;

    for (size_t bit = VOL_MIN; bit < VOL_MAX && bit < volume; bit++) {
        val = (val >> 1) | (1 << (VOL_MAX-1));
    }

    *((uint32_t*)(ioctrl + 0x80)) = val;
}
