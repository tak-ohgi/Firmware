/****************************************************************************
 *
 *   Copyright (c) 2012-2014 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file config.c
 * @author Lorenz Meier <lm@inf.ethz.ch>
 * @author Julian Oes <joes@student.ethz.ch>
 *
 * config tool. Takes the device name as the first parameter.
 */

#include <px4_config.h>
#include <px4_log.h>
#include <px4_module.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <arch/board/board.h>

#include <drivers/drv_gyro.h>
#include <drivers/drv_accel.h>
#include <drivers/drv_mag.h>
#include <drivers/drv_device.h>

#include "systemlib/systemlib.h"
#include "systemlib/param/param.h"

__EXPORT int config_main(int argc, char *argv[]);

static int	do_gyro(int argc, char *argv[]);
static int	do_accel(int argc, char *argv[]);
static int	do_mag(int argc, char *argv[]);
static int	do_device(int argc, char *argv[]);
static void	print_usage(void);

int
config_main(int argc, char *argv[])
{
	bool is_device_cmd = argc >= 2 && (!strcmp(argv[1], "block") || !strcmp(argv[1], "unblock"));

	if (argc >= 3) {
		if (!is_device_cmd && !strncmp(argv[2], "/dev/gyro", 9)) {
			return do_gyro(argc - 1, argv + 1);

		} else if (!is_device_cmd && !strncmp(argv[2], "/dev/accel", 10)) {
			return do_accel(argc - 1, argv + 1);

		} else if (!is_device_cmd && !strncmp(argv[2], "/dev/mag", 8)) {
			return do_mag(argc - 1, argv + 1);

		} else {
			return do_device(argc - 1, argv + 1);
		}
	}

	print_usage();
	return 1;
}
static void
print_usage(void)
{
	PRINT_MODULE_DESCRIPTION("Configure a sensor driver (sampling & publication rate, range, etc.)");

	PRINT_MODULE_USAGE_NAME("config", "command");
	PRINT_MODULE_USAGE_PARAM_COMMENT("The <file:dev> argument is typically one of /dev/{gyro,accel,mag}i");

	PRINT_MODULE_USAGE_COMMAND_DESCR("block", "Block sensor topic publication");
	PRINT_MODULE_USAGE_ARG("<file:dev>", "Sensor device file", false);
	PRINT_MODULE_USAGE_COMMAND_DESCR("unblock", "Unblock sensor topic publication");
	PRINT_MODULE_USAGE_ARG("<file:dev>", "Sensor device file", false);

	PRINT_MODULE_USAGE_COMMAND_DESCR("sampling", "Set sensor sampling rate");
	PRINT_MODULE_USAGE_ARG("<file:dev> <rate>", "Sensor device file and sampling rate in Hz", false);
	PRINT_MODULE_USAGE_COMMAND_DESCR("rate", "Set sensor publication rate");
	PRINT_MODULE_USAGE_ARG("<file:dev> <rate>", "Sensor device file and publication rate in Hz", false);
	PRINT_MODULE_USAGE_COMMAND_DESCR("range", "Set sensor measurement range");
	PRINT_MODULE_USAGE_ARG("<file:dev> <rate>", "Sensor device file and range", false);
	PRINT_MODULE_USAGE_COMMAND_DESCR("check", "Perform sensor self-test (and print info)");
	PRINT_MODULE_USAGE_ARG("<file:dev>", "Sensor device file", false);
}

static int
do_device(int argc, char *argv[])
{
	if (argc < 2) {
		print_usage();
		return 1;
	}

	int	fd;

	fd = open(argv[1], 0);

	if (fd < 0) {
		PX4_ERR("open %s failed (%i)", argv[1], errno);
		return 1;

	} else {

		int	ret;

		if (argc == 2 && !strcmp(argv[0], "block")) {

			/* disable the device publications */
			ret = ioctl(fd, DEVIOCSPUBBLOCK, 1);

			if (ret) {
				PX4_ERR("uORB publications could not be blocked");
				return 1;
			}

		} else if (argc == 2 && !strcmp(argv[0], "unblock")) {

			/* enable the device publications */
			ret = ioctl(fd, DEVIOCSPUBBLOCK, 0);

			if (ret) {
				PX4_ERR("uORB publications could not be unblocked");
				return 1;
			}

		} else {
			print_usage();
			return 1;
		}
	}

	return 0;
}

static int
do_gyro(int argc, char *argv[])
{
	int	fd;

	fd = open(argv[1], 0);

	if (fd < 0) {
		PX4_ERR("open %s failed (%i)", argv[1], errno);
		return 1;

	} else {

		int ret;

		if (argc == 3 && !strcmp(argv[0], "sampling")) {

			/* set the gyro internal sampling rate up to at least i Hz */
			ret = ioctl(fd, GYROIOCSSAMPLERATE, strtoul(argv[2], NULL, 0));

			if (ret) {
				PX4_ERR("sampling rate could not be set");
				return 1;
			}

		} else if (argc == 3 && !strcmp(argv[0], "rate")) {

			/* set the driver to poll at i Hz */
			ret = ioctl(fd, SENSORIOCSPOLLRATE, strtoul(argv[2], NULL, 0));

			if (ret) {
				PX4_ERR("pollrate could not be set");
				return 1;
			}

		} else if (argc == 3 && !strcmp(argv[0], "range")) {

			/* set the range to i dps */
			ret = ioctl(fd, GYROIOCSRANGE, strtoul(argv[2], NULL, 0));

			if (ret) {
				PX4_ERR("range could not be set");
				return 1;
			}

		} else if (argc == 2 && !strcmp(argv[0], "check")) {
			ret = ioctl(fd, GYROIOCSELFTEST, 0);

			if (ret) {
				PX4_WARN("gyro self test FAILED! Check calibration:");
				struct gyro_calibration_s scale;
				ret = ioctl(fd, GYROIOCGSCALE, (long unsigned int)&scale);

				if (ret) {
					PX4_ERR("failed getting gyro scale");
					return 1;
				}

				PX4_INFO("offsets: X: % 9.6f Y: % 9.6f Z: % 9.6f", (double)scale.x_offset, (double)scale.y_offset,
					 (double)scale.z_offset);
				PX4_INFO("scale:   X: % 9.6f Y: % 9.6f Z: % 9.6f", (double)scale.x_scale, (double)scale.y_scale, (double)scale.z_scale);

			} else {
				PX4_INFO("gyro calibration and self test OK");
			}

		} else {
			print_usage();
			return 1;
		}

		int srate = ioctl(fd, GYROIOCGSAMPLERATE, 0);
		int prate = ioctl(fd, SENSORIOCGPOLLRATE, 0);
		int range = ioctl(fd, GYROIOCGRANGE, 0);
		int id = ioctl(fd, DEVIOCGDEVICEID, 0);
		int32_t calibration_id = 0;

		param_get(param_find("CAL_GYRO0_ID"), &(calibration_id));

		PX4_INFO("gyro: \n\tdevice id:\t0x%X\t(calibration is for device id 0x%X)\n\tsample rate:\t%d Hz\n\tread rate:\t%d Hz\n\trange:\t%d dps",
			 id, calibration_id, srate, prate, range);

		close(fd);
	}

	return 0;
}

static int
do_mag(int argc, char *argv[])
{
	int fd;

	fd = open(argv[1], 0);

	if (fd < 0) {
		PX4_ERR("open %s failed (%i)", argv[1], errno);
		return 1;

	} else {

		int ret;

		if (argc == 3 && !strcmp(argv[0], "sampling")) {

			/* set the mag internal sampling rate up to at least i Hz */
			ret = ioctl(fd, MAGIOCSSAMPLERATE, strtoul(argv[2], NULL, 0));

			if (ret) {
				PX4_ERR("sampling rate could not be set");
				return 1;
			}

		} else if (argc == 3 && !strcmp(argv[0], "rate")) {

			/* set the driver to poll at i Hz */
			ret = ioctl(fd, SENSORIOCSPOLLRATE, strtoul(argv[2], NULL, 0));

			if (ret) {
				PX4_ERR("pollrate could not be set");
				return 1;
			}

		} else if (argc == 3 && !strcmp(argv[0], "range")) {

			/* set the range to i G */
			ret = ioctl(fd, MAGIOCSRANGE, strtoul(argv[2], NULL, 0));

			if (ret) {
				PX4_ERR("range could not be set");
				return 1;
			}

		} else if (argc == 2 && !strcmp(argv[0], "check")) {
			ret = ioctl(fd, MAGIOCSELFTEST, 0);

			if (ret) {
				PX4_WARN("mag self test FAILED! Check calibration:");
				struct mag_calibration_s scale;
				ret = ioctl(fd, MAGIOCGSCALE, (long unsigned int)&scale);

				if (ret) {
					PX4_ERR("failed getting mag scale");
					return 1;
				}

				PX4_INFO("offsets: X: % 9.6f Y: % 9.6f Z: % 9.6f", (double)scale.x_offset, (double)scale.y_offset,
					 (double)scale.z_offset);
				PX4_INFO("scale:   X: % 9.6f Y: % 9.6f Z: % 9.6f", (double)scale.x_scale, (double)scale.y_scale, (double)scale.z_scale);

			} else {
				PX4_INFO("mag calibration and self test OK");
			}

		} else {
			print_usage();
			return 1;
		}

		int srate = ioctl(fd, MAGIOCGSAMPLERATE, 0);
		int prate = ioctl(fd, SENSORIOCGPOLLRATE, 0);
		int range = ioctl(fd, MAGIOCGRANGE, 0);
		int id = ioctl(fd, DEVIOCGDEVICEID, 0);
		int32_t calibration_id = 0;

		param_get(param_find("CAL_MAG0_ID"), &(calibration_id));

		PX4_INFO("mag: \n\tdevice id:\t0x%X\t(calibration is for device id 0x%X)\n\tsample rate:\t%d Hz\n\tread rate:\t%d Hz\n\trange:\t%d Ga",
			 id, calibration_id, srate, prate, range);

		close(fd);
	}

	return 0;
}

static int
do_accel(int argc, char *argv[])
{
	int	fd;

	fd = open(argv[1], 0);

	if (fd < 0) {
		PX4_ERR("open %s failed (%i)", argv[1], errno);
		return 1;

	} else {

		int ret;

		if (argc == 3 && !strcmp(argv[0], "sampling")) {

			/* set the accel internal sampling rate up to at least i Hz */
			ret = ioctl(fd, ACCELIOCSSAMPLERATE, strtoul(argv[2], NULL, 0));

			if (ret) {
				PX4_ERR("sampling rate could not be set");
				return 1;
			}

		} else if (argc == 3 && !strcmp(argv[0], "rate")) {

			/* set the driver to poll at i Hz */
			ret = ioctl(fd, SENSORIOCSPOLLRATE, strtoul(argv[2], NULL, 0));

			if (ret) {
				PX4_ERR("pollrate could not be set");
				return 1;
			}

		} else if (argc == 3 && !strcmp(argv[0], "range")) {

			/* set the range to i G */
			ret = ioctl(fd, ACCELIOCSRANGE, strtoul(argv[2], NULL, 0));

			if (ret) {
				PX4_ERR("range could not be set");
				return 1;
			}

		} else if (argc == 2 && !strcmp(argv[0], "check")) {
			ret = ioctl(fd, ACCELIOCSELFTEST, 0);

			if (ret) {
				PX4_WARN("accel self test FAILED! Check calibration:");
				struct accel_calibration_s scale;
				ret = ioctl(fd, ACCELIOCGSCALE, (long unsigned int)&scale);

				if (ret) {
					PX4_ERR("failed getting accel scale");
					return 1;
				}

				PX4_INFO("offsets: X: % 9.6f Y: % 9.6f Z: % 9.6f", (double)scale.x_offset, (double)scale.y_offset,
					 (double)scale.z_offset);
				PX4_INFO("scale:   X: % 9.6f Y: % 9.6f Z: % 9.6f", (double)scale.x_scale, (double)scale.y_scale, (double)scale.z_scale);

			} else {
				PX4_INFO("accel calibration and self test OK");
			}

		} else {
			print_usage();
			return 1;
		}

		int srate = ioctl(fd, ACCELIOCGSAMPLERATE, 0);
		int prate = ioctl(fd, SENSORIOCGPOLLRATE, 0);
		int range = ioctl(fd, ACCELIOCGRANGE, 0);
		int id = ioctl(fd, DEVIOCGDEVICEID, 0);
		int32_t calibration_id = 0;

		param_get(param_find("CAL_ACC0_ID"), &(calibration_id));

		PX4_INFO("accel: \n\tdevice id:\t0x%X\t(calibration is for device id 0x%X)\n\tsample rate:\t%d Hz\n\tread rate:\t%d Hz\n\trange:\t%d G",
			 id, calibration_id, srate, prate, range);

		close(fd);
	}

	return 0;
}
