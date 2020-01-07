
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <cobalt/stdio.h>

#include <alchemy/task.h>
#include <alchemy/timer.h>

#include "audio_player_board.h"

#include "io_utils.h"

#include "audio_player_board.h"



static int snd_fd;

static int rtsnd_fd;

static void *ioctrls;

int init_board(void)
{




    /* Ouverture du driver RTDM */
    rtsnd_fd = open("/dev/rtdm/snd", O_RDWR);
    if (rtsnd_fd < 0) {
        perror("Opening /dev/rtdm/snd");
        exit(EXIT_FAILURE);
    }

    snd_fd = rtsnd_fd;

    ioctrls = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, rtsnd_fd, 0);
    if (ioctrls == MAP_FAILED) {
        perror("Mapping real-time sound file descriptor");
        exit(EXIT_FAILURE);
    }

    return 0;
}

int clean_board(void)
{
    close(rtsnd_fd);
    if (munmap(ioctrls, 4096) == -1) {
        perror("Unmapping");
        exit(EXIT_FAILURE);
    }
    return 0;
}

ssize_t write_samples(const void *buf, size_t count)
{
    return write(snd_fd, buf, count);
}

ssize_t read_samples(void *buf, size_t count)
{
    return read(snd_fd, buf, count);
}

void set_display_time(uint8_t minutes, uint8_t seconds, uint8_t centisecs)
{
    display_time(ioctrls, minutes, seconds, centisecs);
}

uint32_t get_buttons(void)
{
    return ~keys(ioctrls);
}

uint32_t get_switches(void)
{
    return switches(ioctrls);
}

void set_display_volume(uint8_t volume)
{
    set_volume_leds(ioctrls, volume);
}


int is_pressed_now(uint32_t buttons_before, uint32_t buttons_now, uint8_t bit)
{
    uint32_t before_bit = buttons_before & bit;
    uint32_t now_bit = buttons_now & bit;

    if (now_bit && !before_bit) return 1;
    else return 0;
}
