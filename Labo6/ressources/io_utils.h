#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stdint.h>

#define MS 1000000

/* Affiche le temps sur les afficheurs 7 segments */
void display_time(void *ioctrl,
                  uint8_t minutes, uint8_t seconds, uint8_t centisecs);

/* Retourne l'état des boutons poussoirs (attention, ils sont actifs bas) */
uint32_t keys(void *ioctrl);

/* Retourne l'état des switchs */
uint32_t switches(void *ioctrl);


/* Affiche le niveau de volume du son sur les leds */
void set_volume_leds(void *ioctrl, uint8_t volume);


#endif // IO_UTILS_H
