#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "audio_utils.h"


int16_t map_volume(int16_t sample, uint8_t volume)
{
    float volume_ratio = (float)volume / VOL_MAX;
    return (int16_t)(sample * volume_ratio);
}

int parse_wav_header(int audio_fd, struct wav_header *wh)
{
    size_t header_size = sizeof(struct wav_header);
    ssize_t read_ret;

    /* Reposition file offset to start of file */
    lseek(audio_fd, 0, SEEK_SET);

    read_ret = read(audio_fd, wh, header_size);
    if (read_ret < header_size) {
        fprintf(stderr, "Couldn't read whole wav header.\n");
        return -1;
    }

    if (wh->file_blkID != WAV_FILE_BLKID) {
        fprintf(stderr, "[%s] Wrong file block ID. Found : 0x%X\n", __func__, wh->file_blkID);
        return -1;
    }

    if (wh->fmt_blkID != WAV_FMT_BLKID) {
        fprintf(stderr, "[%s] Wrong audio format block ID\n", __func__);
        return -1;
    }

    if (wh->data_blkID != WAV_DATA_BLKID) {
        fprintf(stderr, "[%s] Wrong data block ID\n", __func__);
        return -1;
    }

    return 0;
}

int copy_wav_data(int audio_fd, const struct wav_file *wf)
{
    size_t to_read = wf->wh.data_size;
    ssize_t read_ret;
    void *audio_datas_p = wf->audio_datas;

    lseek(audio_fd, 44, SEEK_SET);
    do {
        read_ret = read(audio_fd, audio_datas_p, to_read);
        if (read_ret < 0) {
            perror("Reading audio data from wav file");
            return -1;
        }
        else if (read_ret == 0 && to_read > 0) {
            fprintf(stderr, "Reached EOF but data remains to be read\n");
            return -1;
        }

        to_read -= read_ret;
        audio_datas_p += read_ret;

    } while (to_read);

    return 0;
}
