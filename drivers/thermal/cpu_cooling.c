/*
 *  linux/drivers/thermal/cpu_cooling.c
 *
 *  Copyright (C) 2012	Samsung Electronics Co., Ltd(http://www.samsung.com)
 *  Copyright (C) 2012  Amit Daniel <amit.kachhap@linaro.org>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/cpu_cooling.h>

/**
 * struct cpufreq_cooling_device - data for cooling device with cpufreq
 * @id: unique integer value corresponding to each cpufreq_cooling_device
 *	registered.
 * @cool_dev: thermal_cooling_device pointer to keep track of the
 *	registered cooling device.
 * @cpufreq_state: integer value representing the current state of cpufreq
 *	cooling	devices.
 * @cpufreq_val: integer value representing the absolute value of the clipped
 *	frequency.
 * @allowed_cpus: all the cpus involved for this cpufreq_cooling_device.
 * @node: list_head to link all cpufreq_cooling_device together.
 *
 * This structure is required for keeping information of each
 * cpufreq_cooling_device registered as a list whose head is represented by
 * cooling_cpufreq_list. In order to prevent corruption of this list a
 * mutex lock cooling_cpufreq_lock is used.
 */
struct cpufreq_cooling_device {
	int id;
	struct thermal_cooling_device *cool_dev;
	unsigned int cpufreq_state;
	unsigned int cpufreq_val;
	struct cpumask allowed_cpus;
	struct list_head node;
};
static LIST_HEAD(cooling_cpufreq_list);
static DEFINE_IDR(cpufreq_idr);
static DEFINE_MUTEX(cooling_cpufreq_lock);

static unsigned int cpufreq_dev_count;

/* notify_table passes value to the CPUFREQ_ADJUST callback function. */
#define NOTIFY_INVALID NULL
static struct cpufreq_cooling_device *notify_device;

/**
 * get_idr - function to get a unique id.
 * @idr: struct idr * handle used to create a id.
 * @id: int * value generated by this function.
 */
static int get_idr(struct idr *idr, int *id)
{
	int ret;

	mutex_lock(&cooling_cpufreq_lock);
	ret = idr_alloc(idr, NULL, 0, 0, GFP_KERNEL);
	mutex_unlock(&cooling_cpufreq_lock);
	if (unlikely(ret < 0))
		return ret;
	*id = ret;
	return 0;
}

/**
 * release_idr - function to free the unique id.
 * @idr: struct idr * handle used for creating the id.
 * @id: int value representing the unique id.
 */
static void release_idr(struct idr *idr, int id)
{
	mutex_lock(&cooling_cpufreq_lock);
	idr_remove(idr, id);
	mutex_unlock(&cooling_cpufreq_lock);
}

/* Below code defines functions to be used for cpufreq as cooling device */

/**
 * is_cpufreq_valid - function to check frequency transitioning capability.
 * @cpu: cpu for which check is needed.
 *
 * This function will check the current state of the system if
 * it is capable of changing the frequency for a given @cpu.
 *
 * Return: 0 if the system is not currently capable of changing
 * the frequency of given cpu. !0 in case the frequency is changeable.
 */
static int is_cpufreq_valid(int cpu)
{
	struct cpufreq_policy policy;
	return !cpufreq_get_policy(&policy, cpu);
}

enum cpufreq_cooling_property {
	GET_LEVEL,
	GET_FREQ,
	GET_MAXL,
};

/**
 * get_property - fetch a property of interest for a give cpu.
 * @cpu: cpu for which the property is required
 * @input: query parameter
 * @output: query return
 * @property: type of query (frequency, level, max level)
 *
 * This is the common function to
 * 1. get maximum cpu cooling states
 * 2. translate frequency to cooling state
 * 3. translate cooling state to frequency
 * Note that the code may be not in good shape
 * but it is written in this way in order to:
 * a) reduce duplicate code as most of the code can be shared.
 * b) make sure the logic is consistent when translating between
 *    cooling states and frequencies.
 *
 * Return: 0 on success, -EINVAL when invalid parameters are passed.
 */
static int get_property(unsigned int cpu, unsigned long input,
	unsigned int* output, enum cpufreq_cooling_property property)
{
	int i, j;
	unsigned long max_level = 0, level = 0;
	unsigned int freq = CPUFREQ_ENTRY_INVALID;
	int descend = -1;
	struct cpufreq_frequency_table *table =
					cpufreq_frequency_get_table(cpu);
	
	if (!output)
		return -EINVAL;

	if (!table)
		return -EINVAL;

	
	for (i = 0; table[i].frequency != CPUFREQ_TABLE_END; i++) {
		/* ignore invalid entries */
		if (table[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;

		/* ignore duplicate entry */
		if (freq == table[i].frequency)
			continue;

		/* get the frequency order */
		if (freq != CPUFREQ_ENTRY_INVALID && descend != -1)
			descend = !!(freq > table[i].frequency);

		freq = table[i].frequency;
		max_level++;
	}

	/* get max level */
	if (property == GET_MAXL) {
		*output = (unsigned int)max_level;
		return 0;
	}

	if (property == GET_FREQ)
		level = descend ? input : (max_level - input -1);


	for (i = 0, j = 0; table[i].frequency != CPUFREQ_TABLE_END; i++) {
		/* ignore invalid entry */
		if (table[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;

		/* ignore duplicate entry */
		if (freq == table[i].frequency)
			continue;

		/* now we have a valid frequency entry */
		freq = table[i].frequency;

		if (property == GET_LEVEL && (unsigned int)input == freq) {
			/* get level by frequency */
			*output = descend ? j : (max_level - j - 1);
			return 0;
		}
		if (property == GET_FREQ && level == j) {
			/* get frequency by level */
			*output = freq;
			return 0;
		}
		j++;
	}
	return -EINVAL;
}

/**
 * cpufreq_cooling_get_level - for a give cpu, return the cooling level.
 * @cpu: cpu for which the level is required
 * @freq: the frequency of interest
 *
 * This function will match the cooling level corresponding to the
 * requested @freq and return it.
 *
 * Return: The matched cooling level on success or THERMAL_CSTATE_INVALID
 * otherwise.
 */
unsigned long cpufreq_cooling_get_level(unsigned int cpu, unsigned int freq)
{
	unsigned int val;

	if (get_property(cpu, (unsigned long)freq, &val, GET_LEVEL))
		return THERMAL_CSTATE_INVALID;
	return (unsigned long)val;
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_get_level);

/**
 * get_cpu_frequency - get the absolute value of frequency from level.
 * @cpu: cpu for which frequency is fetched.
 * @level: cooling level
 *
 * This function matches cooling level with frequency. Based on a cooling level
 * of frequency, equals cooling state of cpu cooling device, it will return
 * the corresponding frequency.
 *	e.g level=0 --> 1st MAX FREQ, level=1 ---> 2nd MAX FREQ, .... etc
 *
 * Return: 0 on error, the corresponding frequency otherwise.
 */
static unsigned int get_cpu_frequency(unsigned int cpu, unsigned long level)
{
	int ret = 0;
	unsigned int freq;

	ret = get_property(cpu, level, &freq, GET_FREQ);
	if (ret)
		return 0;
	return freq;
}

/**
 * cpufreq_apply_cooling - function to apply frequency clipping.
 * @cpufreq_device: cpufreq_cooling_device pointer containing frequency
 *	clipping data.
 * @cooling_state: value of the cooling state.
 *
 * Function used to make sure the cpufreq layer is aware of current thermal
 * limits. The limits are applied by updating the cpufreq policy.
 *
 * Return: 0 on success, an error code otherwise (-EINVAL in case wrong
 * cooling state).
 */
static int cpufreq_apply_cooling(struct cpufreq_cooling_device *cpufreq_device,
				unsigned long cooling_state)
{
	unsigned int cpuid, clip_freq;
	struct cpumask *mask = &cpufreq_device->allowed_cpus;
	unsigned int cpu = cpumask_any(mask);


	/* Check if the old cooling action is same as new cooling action */
	if (cpufreq_device->cpufreq_state == cooling_state)
		return 0;

	clip_freq = get_cpu_frequency(cpu, cooling_state);
	if (!clip_freq)
		return -EINVAL;

	cpufreq_device->cpufreq_state = cooling_state;
	cpufreq_device->cpufreq_val = clip_freq;
	notify_device = cpufreq_device;

	for_each_cpu(cpuid, mask) {
		if (is_cpufreq_valid(cpuid))
			cpufreq_update_policy(cpuid);
	}

	notify_device = NOTIFY_INVALID;

	return 0;
}

/**
 * cpufreq_thermal_notifier - notifier callback for cpufreq policy change.
 * @nb:	struct notifier_block * with callback info.
 * @event: value showing cpufreq event for which this function invoked.
 * @data: callback-specific data
 */
static int cpufreq_thermal_notifier(struct notifier_block *nb,
					unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	unsigned long max_freq = 0;

	if (event != CPUFREQ_ADJUST || notify_device == NOTIFY_INVALID)
		return 0;

	if (cpumask_test_cpu(policy->cpu, &notify_device->allowed_cpus))
		max_freq = notify_device->cpufreq_val;

	/* Never exceed user_policy.max*/
	if (max_freq > policy->user_policy.max)
		max_freq = policy->user_policy.max;

	if (policy->max != max_freq)
		cpufreq_verify_within_limits(policy, 0, max_freq);

	return 0;
}

/*
 * cpufreq cooling device callback functions are defined below
 */

/**
 * cpufreq_get_max_state - callback function to get the max cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the max cooling state.
 */
static int cpufreq_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;
	struct cpumask *mask = &cpufreq_device->allowed_cpus;
	unsigned int cpu;
	unsigned int count = 0;
	int ret;

	cpu = cpumask_any(mask);

	ret = get_property(cpu, 0, &count, GET_MAXL);

	if (count > 0)
		*state = count;

	return ret;
}

/**
 * cpufreq_get_cur_state - callback function to get the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the current cooling state.
 */
static int cpufreq_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;

	*state = cpufreq_device->cpufreq_state;
	return 0;
}

/**
 * cpufreq_set_cur_state - callback function to set the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: set this variable to the current cooling state.
 */
static int cpufreq_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;

	return cpufreq_apply_cooling(cpufreq_device, state);
}

/* Bind cpufreq callbacks to thermal cooling device ops */
static struct thermal_cooling_device_ops const cpufreq_cooling_ops = {
	.get_max_state = cpufreq_get_max_state,
	.get_cur_state = cpufreq_get_cur_state,
	.set_cur_state = cpufreq_set_cur_state,
};

/* Notifier for cpufreq policy change */
static struct notifier_block thermal_cpufreq_notifier_block = {
	.notifier_call = cpufreq_thermal_notifier,
};

/**
 * cpufreq_cooling_register - function to create cpufreq cooling device.
 * @clip_cpus: cpumask of cpus where the frequency constraints will happen.
 */
struct thermal_cooling_device *cpufreq_cooling_register(
	const struct cpumask *clip_cpus)
{
	struct thermal_cooling_device *cool_dev;
	struct cpufreq_cooling_device *cpufreq_dev = NULL;
	unsigned int min = 0, max = 0;
	char dev_name[THERMAL_NAME_LENGTH];
	int ret = 0, i;
	struct cpufreq_policy policy;

	/*Verify that all the clip cpus have same freq_min, freq_max limit*/
	for_each_cpu(i, clip_cpus) {
		/*continue if cpufreq policy not found and not return error*/
		if (!cpufreq_get_policy(&policy, i))
			continue;
		if (min == 0 && max == 0) {
			min = policy.cpuinfo.min_freq;
			max = policy.cpuinfo.max_freq;
		} else {
			if (min != policy.cpuinfo.min_freq ||
				max != policy.cpuinfo.max_freq)
				return ERR_PTR(-EINVAL);
		}
	}
	cpufreq_dev = kzalloc(sizeof(struct cpufreq_cooling_device),
			GFP_KERNEL);
	if (!cpufreq_dev)
		return ERR_PTR(-ENOMEM);

	cpumask_copy(&cpufreq_dev->allowed_cpus, clip_cpus);

	ret = get_idr(&cpufreq_idr, &cpufreq_dev->id);
	if (ret) {
		kfree(cpufreq_dev);
		return ERR_PTR(-EINVAL);
	}

	sprintf(dev_name, "thermal-cpufreq-%d", cpufreq_dev->id);

	cool_dev = thermal_cooling_device_register(dev_name, cpufreq_dev,
						&cpufreq_cooling_ops);
	if (!cool_dev) {
		release_idr(&cpufreq_idr, cpufreq_dev->id);
		kfree(cpufreq_dev);
		return ERR_PTR(-EINVAL);
	}
	cpufreq_dev->cool_dev = cool_dev;
	cpufreq_dev->cpufreq_state = 0;
	mutex_lock(&cooling_cpufreq_lock);

	/* Register the notifier for first cpufreq cooling device */
	if (cpufreq_dev_count == 0)
		cpufreq_register_notifier(&thermal_cpufreq_notifier_block,
						CPUFREQ_POLICY_NOTIFIER);
	cpufreq_dev_count++;

	mutex_unlock(&cooling_cpufreq_lock);
	return cool_dev;
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_register);

/**
 * cpufreq_cooling_unregister - function to remove cpufreq cooling device.
 * @cdev: thermal cooling device pointer.
 */
void cpufreq_cooling_unregister(struct thermal_cooling_device *cdev)
{
	struct cpufreq_cooling_device *cpufreq_dev = cdev->devdata;

	mutex_lock(&cooling_cpufreq_lock);
	cpufreq_dev_count--;

	/* Unregister the notifier for the last cpufreq cooling device */
	if (cpufreq_dev_count == 0) {
		cpufreq_unregister_notifier(&thermal_cpufreq_notifier_block,
					CPUFREQ_POLICY_NOTIFIER);
	}
	mutex_unlock(&cooling_cpufreq_lock);

	thermal_cooling_device_unregister(cpufreq_dev->cool_dev);
	release_idr(&cpufreq_idr, cpufreq_dev->id);
	kfree(cpufreq_dev);
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_unregister);
