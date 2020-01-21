#ifndef AUDIO_PLAYER_BOARD_H
#define AUDIO_PLAYER_BOARD_H

#include <unistd.h>
#include <stdint.h>

#include "audio_utils.h"

/* Bit offsets for keys */
#define KEY0    0x1
#define KEY1    0x2
#define KEY2    0x4
#define KEY3    0x8

/* Bit offsets for switches */
#define SW0     0x1

/* Playback and record states */
#define PLAYING     0
#define PAUSED      1
#define STOPPED 	2
#define RECORDING   3
#define END			4

/* Playback seeking modes */
#define SEEK_NONE   0
#define SEEK_FWD    1
#define SEEK_RWD    2

/* Audio codec buffer size in bytes (in one direction) */
#define AUDIO_CODEC_BUF_SIZE    1024 // 128 * 4 bytes * 2 channels

/* Audio codec configuration */
#define AUDIO_CODEC_SAMPLE_RATE 48000 // in hertz
#define AUDIO_CODEC_SAMPLE_SIZE 2 // in bytes
#define AUDIO_CODEC_CHANNELS    2

#define BYTES_EVERY_10_SEC    (10*(AUDIO_CODEC_SAMPLE_RATE*AUDIO_CODEC_SAMPLE_SIZE*AUDIO_CODEC_CHANNELS))

/**
 * @brief Initializes the board
 * @return 0 if everything went well, another number else.
 *
 * This function should be called at the beginning of the main(),
 * before any access is made to the various peripherals
 */
int init_board(void);

/**
 * @brief Cleans the board resources
 * @return 0 if everything went well, another number else.
 *
 * This function should be called before leaving the process.
 */
int clean_board(void);

/**
 * @brief Writes samples to the audio channel
 * @param buf A pointer to the data to send
 * @param count The number of bytes to send
 * @return The actual number of bytes sent
 */
ssize_t write_samples(const void *buf, size_t count);

/**
 * @brief Reads samples from the audio codec
 * @param buf A pointer to a buffer where audio samples are written
 * @param count The size in bytes of the buffer
 * @return The actual number of bytes read. Returns a negative value if an error occured
 */
ssize_t read_samples(void *buf, size_t count);

/**
 * @brief Displays the elapsed time on the 7-segment display
 * @param minutes Number of minutes
 * @param seconds Number of seconds
 * @param centisecs Number of hundredth of a second
 */
void set_display_time(uint8_t minutes, uint8_t seconds, uint8_t centisecs);

/**
 * @brief Gets the status of buttons
 * @return The status of the 4 buttons.
 *
 * 1 means the button is pressed
 */
uint32_t get_buttons(void);

/**
 * @brief Gets the status of the switches
 * @return The status of the switches
 */
uint32_t get_switches(void);

/**
 * @brief Sets the display corresponding to the volume
 * @param volume The sound volume (0-10)
 *
 * The volume can be any number in [0, 10].
 */
void set_display_volume(uint8_t volume);


/* Retourne 1 ou 0 si un bouton viens d'être appuyé en fonction de l'état
 * précédant et l'état courant des boutons poussoirs */
/**
 * @brief Checks if a button was just pressed
 * @param buttons_before The previous value of the buttons
 * @param buttons_now The current value of the buttons
 * @param bit The button of interest
 * @return 1 if the button was just pressed, 0 else
 */
int is_pressed_now(uint32_t buttons_before, uint32_t buttons_now, uint8_t bit);

#endif // AUDIO_PLAYER_BOARD_H
