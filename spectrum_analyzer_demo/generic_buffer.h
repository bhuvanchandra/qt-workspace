#ifndef GENERIC_BUFFER_H
#define GENERIC_BUFFER_H

#include <stdbool.h>

struct generic_buffer {
    unsigned long num_loops; /* TODO: This is not required as we try to do it continously */
    unsigned long buf_len;
    unsigned long timedelay;
    bool noevents;
    bool notrigger;
    char *trigger_name;
    char *device_name;
    char *dev_dir_name;
    char *buf_dir_name;
    char *buffer_access;
    char *data;
    int scan_size;
    int num_channels;
    int fp;
    bool done_init;
    struct iio_channel_info *channels;
};

struct generic_buffer sa_gb;

/**
 * size_from_channelarray() - calculate the storage size of a scan
 * @channels:		the channel info array
 * @num_channels:	number of channels
 *
 * Has the side effect of filling the channels[i].location values used
 * in processing the buffer output.
 **/
int size_from_channelarray(struct iio_channel_info *channels, int num_channels);

int iio_buf_init(struct generic_buffer *sa_gb);
void iio_buf_free(struct generic_buffer *sa_gb);
void print1byte(uint8_t input, struct iio_channel_info *info);
void print2byte(uint16_t input, struct iio_channel_info *info);
void print4byte(uint32_t input, struct iio_channel_info *info);
void print8byte(uint64_t input, struct iio_channel_info *info);

/**
 * process_scan() - print out the values in SI units
 * @data:		pointer to the start of the scan
 * @channels:		information about the channels.
 *			Note: size_from_channelarray must have been called first
 *			      to fill the location offsets.
 * @num_channels:	number of channels
 **/
void process_scan(char *data, struct iio_channel_info *channels, int num_channels);

#endif // GENERIC_BUFFER_H
