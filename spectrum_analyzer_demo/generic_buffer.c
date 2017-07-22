/* Industrialio buffer test code.
 *
 * Copyright (c) 2008 Jonathan Cameron
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is primarily intended as an example application.
 * Reads the current buffer setup from sysfs and starts a short capture
 * from the specified device, pretty printing the result after appropriate
 * conversion.
 *
 * Command line parameters
 * generic_buffer -n <device_name> -t <trigger_name>
 * If trigger name is not specified the program assumes you want a dataready
 * trigger associated with the device and goes looking for it.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <linux/types.h>
#include <string.h>
#include <poll.h>
#include <endian.h>
#include <getopt.h>
#include <inttypes.h>
#include "iio_utils.h"
#include "generic_buffer.h"

int size_from_channelarray(struct iio_channel_info *channels, int num_channels)
{
	int bytes = 0;
	int i = 0;

	while (i < num_channels) {
		if (bytes % channels[i].bytes == 0)
			channels[i].location = bytes;
		else
			channels[i].location = bytes - bytes % channels[i].bytes
					       + channels[i].bytes;

		bytes = channels[i].location + channels[i].bytes;
		i++;
	}

	return bytes;
}

void print1byte(uint8_t input, struct iio_channel_info *info)
{
	/*
	 * Shift before conversion to avoid sign extension
	 * of left aligned data
	 */
	input >>= info->shift;
	input &= info->mask;
	if (info->is_signed) {
		int8_t val = (int8_t)(input << (8 - info->bits_used)) >>
			     (8 - info->bits_used);
        printf("%05f ", ((float)val + info->offset) * info->scale);
	} else {
        printf("%05f ", ((float)input + info->offset) * info->scale);
	}
}

void print2byte(uint16_t input, struct iio_channel_info *info)
{
	/* First swap if incorrect endian */
	if (info->be)
		input = be16toh(input);
	else
		input = le16toh(input);

	/*
	 * Shift before conversion to avoid sign extension
	 * of left aligned data
	 */
	input >>= info->shift;
	input &= info->mask;
	if (info->is_signed) {
		int16_t val = (int16_t)(input << (16 - info->bits_used)) >>
			      (16 - info->bits_used);
        printf("%05f ", ((float)val + info->offset) * info->scale);
	} else {
        printf("%05f ", ((float)input + info->offset) * info->scale);
	}
}

void print4byte(uint32_t input, struct iio_channel_info *info)
{
	/* First swap if incorrect endian */
	if (info->be)
		input = be32toh(input);
	else
		input = le32toh(input);

	/*
	 * Shift before conversion to avoid sign extension
	 * of left aligned data
	 */
	input >>= info->shift;
	input &= info->mask;
	if (info->is_signed) {
		int32_t val = (int32_t)(input << (32 - info->bits_used)) >>
			      (32 - info->bits_used);
        printf("%05f ", ((float)val + info->offset) * info->scale);
	} else {
        printf("%05f ", ((float)input + info->offset) * info->scale);
	}
}

void print8byte(uint64_t input, struct iio_channel_info *info)
{
	/* First swap if incorrect endian */
	if (info->be)
		input = be64toh(input);
	else
		input = le64toh(input);

	/*
	 * Shift before conversion to avoid sign extension
	 * of left aligned data
	 */
	input >>= info->shift;
	input &= info->mask;
	if (info->is_signed) {
		int64_t val = (int64_t)(input << (64 - info->bits_used)) >>
			      (64 - info->bits_used);
		/* special case for timestamp */
		if (info->scale == 1.0f && info->offset == 0.0f)
            printf("%" PRId64 " ", val);
		else
            printf("%05f ",
			       ((float)val + info->offset) * info->scale);
	} else {
        printf("%05f ", ((float)input + info->offset) * info->scale);
	}
}

void process_scan(char *data, struct iio_channel_info *channels, int num_channels)
{
    int k = 0;

    for (k = 0; k < num_channels; k++)
		switch (channels[k].bytes) {
			/* only a few cases implemented so far */
		case 1:
            print1byte(*(uint8_t *)(data + channels[k].location), &channels[k]);
			break;
		case 2:
            print2byte(*(uint16_t *)(data + channels[k].location), &channels[k]);
			break;
		case 4:
            print4byte(*(uint32_t *)(data + channels[k].location), &channels[k]);
			break;
		case 8:
            print8byte(*(uint64_t *)(data + channels[k].location), &channels[k]);
			break;
		default:
			break;
		}
    printf("\n");
}

int iio_buf_init(struct generic_buffer *sa_gb)
{
    int dev_num, trig_num, ret;

    if (!sa_gb->device_name) {
        printf("Device name not set\n");
        return -1;
    }

    /* Find the device requested */
    dev_num = find_type_by_name(sa_gb->device_name, "iio:device");
    if (dev_num < 0) {
        printf("Failed to find the %s\n", sa_gb->device_name);
        return dev_num;
    }

    printf("iio device number being used is %d\n", dev_num);

    ret = asprintf(&sa_gb->dev_dir_name, "%siio:device%d", iio_dir, dev_num);
    if (ret < 0)
        return -ENOMEM;

    if (!sa_gb->notrigger) {
        if (!sa_gb->trigger_name) {
            /*
             * Build the trigger name. If it is device associated
             * its name is <device_name>_dev[n] where n matches
             * the device number found above.
             */
            ret = asprintf(&sa_gb->trigger_name,
                       "%s-dev%d", sa_gb->device_name, dev_num);
            if (ret < 0) {
                ret = -ENOMEM;
                return ret;
            }
        }

        /* Verify the trigger exists */
        trig_num = find_type_by_name(sa_gb->trigger_name, "trigger");
        if (trig_num < 0) {
            printf("Failed to find the trigger %s\n", sa_gb->trigger_name);
            return trig_num;
        }
        printf("iio trigger number being used is %d\n", trig_num);

    } else {
        printf("trigger-less mode selected\n");
    }

    /*
     * Parse the files in scan_elements to identify what channels are
     * present
     */
    ret = build_channel_array(sa_gb->dev_dir_name, &sa_gb->channels, &sa_gb->num_channels);
    if (ret) {
        printf("Problem reading scan element information\n"
            "diag %s\n", sa_gb->dev_dir_name);
        return ret;
    }

    if (!sa_gb->num_channels) {
        printf("No channels are enabled, we have nothing to scan.\n");
        printf("Enable channels manually in FORMAT_SCAN_ELEMENTS_DIR/*_en and try again.\n");
        ret = -ENOENT;
        return ret;
    }

    /*
     * Construct the directory name for the associated buffer.
     * As we know that the lis3l02dq has only one buffer this may
     * be built rather than found.
     */
    ret = asprintf(&sa_gb->buf_dir_name,
               "%siio:device%d/buffer", iio_dir, dev_num);
    if (ret < 0) {
        ret = -ENOMEM;
        return ret;
    }

    if (!sa_gb->notrigger) {
        printf("%s %s\n", sa_gb->dev_dir_name, sa_gb->trigger_name);
        /*
         * Set the device trigger to be the data ready trigger found
         * above
         */
        ret = write_sysfs_string_and_verify("trigger/current_trigger",
                            sa_gb->dev_dir_name,
                            sa_gb->trigger_name);
        if (ret < 0) {
            printf("Failed to write current_trigger file\n");
            return ret;
        }
    }

    /* Setup ring buffer parameters */
    ret = write_sysfs_int("length", sa_gb->buf_dir_name, sa_gb->buf_len);
    if (ret < 0) {
        printf("Failed to write 'length' file\n");
        return ret;
    }

    /* Enable the buffer */
    ret = write_sysfs_int("enable", sa_gb->buf_dir_name, 1);
    if (ret < 0) {
        printf("Failed to enable buffer: %s\n", strerror(-ret));
        return ret;
    }

    sa_gb->scan_size = size_from_channelarray(sa_gb->channels, sa_gb->num_channels);
    sa_gb->data = (char *)malloc(sa_gb->scan_size * sa_gb->buf_len);
    if (!sa_gb->data) {
        ret = -ENOMEM;
        return ret;
    }

    ret = asprintf(&sa_gb->buffer_access, "/dev/iio:device%d", dev_num);
    if (ret < 0) {
        ret = -ENOMEM;
        return ret;
    }

    /* Attempt to open non blocking the access dev */
    sa_gb->fp = open(sa_gb->buffer_access, O_RDONLY | O_NONBLOCK);
    if (sa_gb->fp == -1) /* TODO: If it isn't there make the node */
        printf("Failed to open %s\n", sa_gb->buffer_access);

    sa_gb->done_init = true;
    return 0;
}

void iio_buf_free(struct generic_buffer *sa_gb)
{
    int ret;

    ret = close(sa_gb->fp);

    /* Stop the buffer */
    ret = write_sysfs_int("enable", sa_gb->buf_dir_name, 0);
    if (ret < 0)
        printf("Failed to disabled\n");

    if (!sa_gb->notrigger)
    /* Disconnect the trigger - just write a dummy name. */
    ret = write_sysfs_string("trigger/current_trigger",
                 sa_gb->dev_dir_name, "NULL");
    if (ret < 0)
        printf("Failed to write to %s\n", sa_gb->dev_dir_name);
}
