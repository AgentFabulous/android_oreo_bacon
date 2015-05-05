/*
 * Copyright (c) 2013-2015, Francisco Franco. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/input.h>
#include <linux/time.h>
#include <linux/slab.h>

// Define max possible value for input boost duration.
#define MAX_INPUT_BOOST_DURATION_MS 10000

#define DEFAULT_INPUT_BOOST_FREQ 1497600	// Frequency to boost to on touch
#define DEFAULT_INPUT_BOOST_DURATION_MS 1000	// How long to boost (ms)

unsigned int input_boost_freq = DEFAULT_INPUT_BOOST_FREQ;
unsigned int input_boost_duration_ms = DEFAULT_INPUT_BOOST_DURATION_MS;

struct kobject *input_boost_kobj;

struct touchboost_inputopen {
	struct input_handle *handle;
	struct work_struct inputopen_work;
} touchboost_inputopen;

/*
 * Use this variable in your governor of choice to calculate when the cpufreq
 * core is allowed to ramp the cpu down after an input event. That logic is done
 * by you, this var only outputs the last time in us an event was captured
 */
static u64 last_input_time = 0;

inline u64 get_input_time(void)
{
	return last_input_time;
}

/* Get input_duration in uS */
inline int get_input_boost_duration(void)
{
	return input_boost_duration_ms * 1000;
}

static void boost_input_event(struct input_handle *handle,
                unsigned int type, unsigned int code, int value)
{
	if ((type == EV_ABS))
		last_input_time = ktime_to_us(ktime_get());
}

static int boost_input_connect(struct input_handler *handler,
                struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (handle == NULL)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = handler->name;

	error = input_register_handle(handle);
	if (error)
		goto err;

	error = input_open_device(handle);
        if (error) {
                input_unregister_handle(handle);
		goto err;
	}

	return 0;

err:
	kfree(handle);
	return error;
}

static void boost_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id boost_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		/* assumption: MT_.._X & MT_.._Y are in the same long */
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
				BIT_MASK(ABS_MT_POSITION_X) |
				BIT_MASK(ABS_MT_POSITION_Y) },
	},
	{ },
};

static struct input_handler boost_input_handler = {
	.event          = boost_input_event,
	.connect        = boost_input_connect,
	.disconnect     = boost_input_disconnect,
	.name           = "input-boost",
	.id_table       = boost_ids,
};

/* Sysfs */
static ssize_t show_input_boost_freq(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", input_boost_freq);
}

static ssize_t store_input_boost_freq(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	input_boost_freq = val;
	return count;
}

static ssize_t show_input_boost_duration_ms(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", input_boost_duration_ms);
}

static ssize_t store_input_boost_duration_ms(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned long val;
	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	input_boost_duration_ms = val > MAX_INPUT_BOOST_DURATION_MS ? MAX_INPUT_BOOST_DURATION_MS : val;
	return count;
}

static struct kobj_attribute input_boost_freq_attr =
	__ATTR(input_boost_freq, 0644, show_input_boost_freq,
		store_input_boost_freq);

static struct kobj_attribute input_boost_duration_ms_attr =
	__ATTR(input_boost_duration_ms, 0644, show_input_boost_duration_ms,
		store_input_boost_duration_ms);

static struct attribute *input_boost_attrs[] = {
	&input_boost_freq_attr.attr,
	&input_boost_duration_ms_attr.attr,
	NULL,
};

static struct attribute_group input_boost_option_group = {
	.attrs = input_boost_attrs,
};
/* Sysfs End */

static int __init init(void)
{
	int ret;

	ret = input_register_handler(&boost_input_handler);
	if (ret) {
		pr_info("touchboost: Unable to register the input handler\n");
		return -ENOMEM;
	}

	/* Setup sysfs stuff */
	input_boost_kobj = kobject_create_and_add("input_boost", kernel_kobj);
	if (input_boost_kobj == NULL) {
		pr_err("touchboost: subsystem register failed\n");
		return -ENOMEM;
	}
	ret = sysfs_create_group(input_boost_kobj, &input_boost_option_group);

	return ret;
}
late_initcall(init);
