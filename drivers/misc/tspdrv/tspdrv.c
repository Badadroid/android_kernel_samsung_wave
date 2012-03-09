 /*
** =========================================================================
** File:
**	 tspdrv.c
**
** Description:
**	 TouchSense Kernel Module main entry-point for GT-P1000 - P1[C|L|N]
**
** P1 Compat Author: Humberto Borba <kernel@humberos.com.br>
**
** Portions Copyright (c) 2010 Immersion Corporation. All Rights Reserved.
**
** This file contains Original Code and/or Modifications of Original Code
** as defined in and that are subject to the GNU Public License v2 -
** (the 'License'). You may not use this file except in compliance with the
** License. You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or contact
** TouchSenseSales@immersion.com.
**
** The Original Code and all software distributed under the License are
** distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
** EXPRESS OR IMPLIED, AND IMMERSION HEREBY DISCLAIMS ALL SUCH WARRANTIES,
** INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
** FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. Please see
** the License for the specific language governing rights and limitations
** under the License.
** =========================================================================
*/

#ifndef __KERNEL__
#define __KERNEL__
#endif
#ifndef MODULE
#define MODULE
#endif
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/timed_output.h>
#include <linux/delay.h>
#include <linux/wakelock.h>

#include "tspdrv.h"
#include "ImmVibeSPI.c"

/* Device name and version information */
/* DO NOT CHANGE - this is auto-generated */
#define VERSION_STR " v3.4.55.8\n"

/* account extra space for future extra digits in version number */
#define VERSION_STR_LEN 16

/* initialized in init_module */
static char g_szDeviceName[(VIBE_MAX_DEVICE_NAME_LENGTH
							+ VERSION_STR_LEN)
							* NUM_ACTUATORS];
/* initialized in init_module */
static size_t g_cchDeviceName;

/* Flag indicating whether the driver is in use */
static char g_bIsPlaying;

/* Buffer to store data sent to SPI */
#define SPI_BUFFER_SIZE (NUM_ACTUATORS * (VIBE_OUTPUT_SAMPLE_SIZE + SPI_HEADER_SIZE))
static int g_bStopRequested = false;
static actuator_samples_buffer g_SamplesBuffer[NUM_ACTUATORS] = {{0}};

/* For QA purposes */
#ifdef QA_TEST
#define FORCE_LOG_BUFFER_SIZE 128
#define TIME_INCREMENT 5
static int g_nTime = 0;
static int g_nForceLogIndex = 0;
static VibeInt8 g_nForceLog[FORCE_LOG_BUFFER_SIZE];
#endif

#if ((LINUX_VERSION_CODE & 0xFFFF00) < KERNEL_VERSION(2, 6, 0))
#error Unsupported Kernel version
#endif

/* Needs to be included after the global variables because it uses them */
#include "VibeOSKernelLinuxTime.c"

/* timed_output */
#if defined(CONFIG_PHONE_P1_GSM)
#define PWM_PERIOD      44540
#define PWM_DUTY_MAX    44500
#define PWM_DUTY_MIN    22250
#elif defined(CONFIG_PHONE_P1_CDMA)
#define PWM_PERIOD      44640
#define PWM_DUTY_MAX    42408
#define PWM_DUTY_MIN    21204
#endif

#define MAX_TIMEOUT     5000 /* 10s */

static struct wake_lock vib_wake_lock;
static struct work_struct work_timer;
static struct hrtimer timer;

static unsigned int pwm_period = PWM_PERIOD;
static unsigned int pwm_duty = 100;
static unsigned int pwm_duty_value = PWM_DUTY_MAX;
static unsigned int multiplier = (PWM_DUTY_MAX - PWM_DUTY_MIN) / 100;
static int pwm_value = 0;
unsigned int g_PWM_duty_max = PWM_PERIOD;
bool isRunning	 = false;



static int set_vibetonz(int timeout)
{
	static bool vib_en = false;
	if(!timeout)
	{
		if(vib_en)
		{
			if (!isRunning)
				return 0;

			vib_en = false;
			isRunning = false;
			s3c_gpio_cfgpin(VIB_PWM, S3C_GPIO_OUTPUT);
			regulator_force_disable(regulator_motor);
			pwm_disable(Immvib_pwm);
			gpio_direction_output(VIB_EN, 0);
		}
	}
	else
	{
		if(!vib_en)
		{
			if (isRunning)
				return 0;

			vib_en = true;
			isRunning = true;
			s3c_gpio_cfgpin(VIB_PWM, S3C_GPIO_SFN(2));
			regulator_enable(regulator_motor);
			pwm_config(Immvib_pwm, pwm_duty_value, pwm_period);
			pwm_enable(Immvib_pwm);

			gpio_direction_output(VIB_EN, 1);
		}
	}
	pwm_value = timeout;
	return 0;
}

void work_timer_func(struct work_struct *work)
{
	set_vibetonz(0);
}

static enum hrtimer_restart vibetonz_timer_func(struct hrtimer *timer)
{
    if (!work_pending(&work_timer))
		schedule_work(&work_timer);

	return HRTIMER_NORESTART;
}

static int get_time_for_vibetonz(struct timed_output_dev *dev)
{
	int remaining;
	if (hrtimer_active(&timer)) {
		ktime_t r = hrtimer_get_remaining(&timer);
		struct timeval t = ktime_to_timeval(r);
		remaining = t.tv_sec * 1000 + t.tv_usec / 1000000;
	} else
		remaining = 0;

	if(pwm_value == -1)
		remaining = -1;

	return remaining;
}

static void enable_vibetonz_from_user(struct timed_output_dev *dev,int value)
{
	hrtimer_cancel(&timer);
	set_vibetonz(value);
	pwm_value = value;

	if (value > 0) {
		if (value > MAX_TIMEOUT)
			value = MAX_TIMEOUT;

		hrtimer_start(&timer,
			ns_to_ktime((u64)value * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);

		pwm_value = 0;
	}
}

static struct timed_output_dev timed_output_vt =
 {
	.name	 = "vibrator",
	.get_time = get_time_for_vibetonz,
	.enable = enable_vibetonz_from_user,
};

static void vibetonz_start(void)
{
	int ret = 0;

	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = vibetonz_timer_func;

	ret = timed_output_dev_register(&timed_output_vt);
	if (ret)
		printk(KERN_ERR "[Vibtonz] timed_output_dev_register is fail \n");
}

static ssize_t p1_vibrator_set_duty(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	sscanf(buf, "%d\n", &pwm_duty);

	if (pwm_duty < 0 || pwm_duty > 100 || !pwm_duty) {
		pr_err("[VIB] %s :invalid interval [0-100]: %d",__FUNCTION__ ,pwm_duty);
		pwm_duty = 100;
	}

	if (pwm_duty >= 0 && pwm_duty <= 100)
		pwm_duty_value = (pwm_duty * multiplier) + PWM_DUTY_MIN;

	printk(KERN_DEBUG "[VIB] %s pwm_duty_value: %d\n",__FUNCTION__ ,pwm_duty_value);
	return size;
}

static ssize_t p1_vibrator_show_duty(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d", pwm_duty);
}

static DEVICE_ATTR(pwm_duty, S_IRUGO | S_IWUGO, p1_vibrator_show_duty, p1_vibrator_set_duty);

static struct attribute *pwm_duty_attributes[] = {
	&dev_attr_pwm_duty.attr,
	NULL
};

static struct attribute_group pwm_duty_group = {
	.attrs = pwm_duty_attributes,
};

static struct miscdevice pwm_duty_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "pwm_duty",
};

/* File IO */
static int open(struct inode *inode, struct file *file);
static int release(struct inode *inode, struct file *file);
static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos);
static ssize_t write(struct file *file, const char *buf, size_t count,
	loff_t *ppos);
static long unlocked_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg);
static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = read,
	.write = write,
	.unlocked_ioctl = unlocked_ioctl,
	.open = open,
	.release = release,
	.llseek =	default_llseek
};

static struct miscdevice miscdev =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = MODULE_NAME,
	.fops = &fops
};

static int suspend(struct platform_device *pdev, pm_message_t state);
static int resume(struct platform_device *pdev);
static struct platform_driver platdrv = {
	.suspend = suspend,
	.resume = resume,
	.driver = {
		.name = MODULE_NAME,
	},
};

static void platform_release(struct device *dev);
static struct platform_device platdev = {
	.name =	 MODULE_NAME,
	/* means that there is only one device */
	.id =	 -1,
	.dev = {
		.platform_data = NULL,
		/* a warning is thrown during rmmod if this is absent */
		.release = platform_release,
	},
};

/* Module info */
MODULE_AUTHOR("Immersion Corporation");
MODULE_DESCRIPTION("TouchSense Kernel Module");
MODULE_LICENSE("GPL v2");

int init_module(void)
{
	int nRet, i; /* initialized below */

	nRet = misc_register(&miscdev);
	if (nRet) {
		printk(KERN_ERR "tspdrv: misc_register failed.\n");
		return nRet;
	}

	nRet = platform_device_register(&platdev);
	if (nRet)
		DbgOut((KERN_ERR "tspdrv: platform_device_register failed.\n"));

	nRet = platform_driver_register(&platdrv);
	if (nRet)
		DbgOut((KERN_ERR "tspdrv: platform_driver_register failed.\n"));

	if (IS_ERR_OR_NULL(regulator_motor)) {
		regulator_motor = regulator_get(NULL, "vcc_motor");
		if (IS_ERR_OR_NULL(regulator_motor)) {
			pr_err("failed to get motor regulator");
			return -EINVAL;
		}
	}

	if (gpio_is_valid(VIB_EN)) {
		gpio_request(VIB_EN, "VIB_EN");
		s3c_gpio_cfgpin(VIB_EN, 1);
		gpio_set_value(VIB_EN, 0);
		s3c_gpio_setpull(VIB_EN, S3C_GPIO_PULL_NONE);
	}

	if (gpio_is_valid(VIB_PWM)) {
		gpio_request(VIB_PWM, "VIB_PWM");
		s3c_gpio_cfgpin(VIB_PWM, S3C_GPIO_OUTPUT);
		gpio_set_value(VIB_PWM, 0);
		s3c_gpio_setpull(VIB_PWM, S3C_GPIO_PULL_NONE);
		gpio_free(VIB_PWM);
	}

	INIT_WORK(&work_timer, work_timer_func);
	ImmVibeSPI_ForceOut_Initialize();
	VibeOSKernelLinuxInitTimer();

	/* Get and concatenate device name and initialize data buffer */
	g_cchDeviceName = 0;
	for (i = 0; i < NUM_ACTUATORS; i++) {
		char *szName = g_szDeviceName + g_cchDeviceName;
		ImmVibeSPI_Device_GetName(i, szName, VIBE_MAX_DEVICE_NAME_LENGTH);

		/* Append version information and get buffer length */
		strcat(szName, VERSION_STR);
		g_cchDeviceName += strlen(szName);

		g_SamplesBuffer[i].nIndexPlayingBuffer = -1; /* Not playing */
		g_SamplesBuffer[i].actuatorSamples[0].nBufferSize = 0;
		g_SamplesBuffer[i].actuatorSamples[1].nBufferSize = 0;
	}

	wake_lock_init(&vib_wake_lock, WAKE_LOCK_SUSPEND, "vib_present");

	if (misc_register(&pwm_duty_device)) {
		printk("%s misc_register(pwm_duty) failed\n", __FUNCTION__);
	} else {
		if (sysfs_create_group(&pwm_duty_device.this_device->kobj, &pwm_duty_group)) {
			printk("failed to create sysfs group for device pwm_duty\n");
		}
	}

	vibetonz_start();
	return 0;
}

void cleanup_module(void)
{
	DbgOut((KERN_DEBUG "tspdrv: cleanup_module.\n"));

	regulator_put(regulator_motor);

	VibeOSKernelLinuxTerminateTimer();
	ImmVibeSPI_ForceOut_Terminate();

	platform_driver_unregister(&platdrv);
	platform_device_unregister(&platdev);

	misc_deregister(&miscdev);
}

static int open(struct inode *inode, struct file *file)
{
	DbgOut((KERN_DEBUG "tspdrv: open.\n"));

	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	return 0;
}

static int release(struct inode *inode, struct file *file)
{
    DbgOut((KERN_DEBUG "tspdrv: release.\n"));

	/*
	** Reset force and stop timer when the driver is closed, to make sure
	** no dangling semaphore remains in the system, especially when the
	** driver is run outside of immvibed for testing purposes.
	*/
	VibeOSKernelLinuxStopTimer();

	/*
	** Clear the variable used to store the magic number to prevent
	** unauthorized caller to write data. TouchSense service is the only
	** valid caller.
	*/
	file->private_data = (void*)NULL;

	module_put(THIS_MODULE);

	return 0;
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	const size_t nBufSize = (g_cchDeviceName > (size_t)(*ppos)) ?
		min(count, g_cchDeviceName - (size_t)(*ppos)) : 0;

	/* End of buffer, exit */
	if (0 == nBufSize)
		return 0;

	if (0 != copy_to_user(buf, g_szDeviceName + (*ppos), nBufSize)) {
		/* Failed to copy all the data, exit */
		DbgOut((KERN_ERR "tspdrv: copy_to_user failed.\n"));
		return 0;
	}

	/* Update file position and return copied buffer size */
	*ppos += nBufSize;
	return nBufSize;
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	int i = 0;
	*ppos = 0; /* file position not used, always set to 0 */

	/*
	** Prevent unauthorized caller to write data.
	** TouchSense service is the only valid caller.
	*/
	if (file->private_data != (void*)TSPDRV_MAGIC_NUMBER) {
		DbgOut((KERN_ERR "tspdrv: unauthorized write.\n"));
		return 0;
	}

	/* Check buffer size */
	if ((count <= SPI_HEADER_SIZE) || (count > SPI_BUFFER_SIZE)) {
		DbgOut((KERN_ERR "tspdrv: invalid write buffer size.\n"));
		return 0;
	}

	while (i < count) {
		int nIndexFreeBuffer;   /* initialized below */

		samples_buffer* pInputBuffer = (samples_buffer*)(&buf[i]);

		if ((i + SPI_HEADER_SIZE) >= count) {
			/*
			** Index is about to go beyond the buffer size.
			** (Should never happen).
			*/
			DbgOut((KERN_EMERG "tspdrv: invalid buffer index.\n"));
		}

		/* Check bit depth */
		if (8 != pInputBuffer->nBitDepth)
			DbgOut((KERN_WARNING "tspdrv: invalid bit depth. Use default value (8).\n"));

		/* The above code not valid if SPI header size is not 3 */
#if (SPI_HEADER_SIZE != 3)
#error "SPI_HEADER_SIZE expected to be 3"
#endif

		/* Check buffer size */
		if ((i + SPI_HEADER_SIZE + pInputBuffer->nBufferSize) > count) {
			/*
			** Index is about to go beyond the buffer size.
			** (Should never happen).
			*/
			DbgOut((KERN_EMERG "tspdrv: invalid data size.\n"));
		}

		/* Check actuator index */
		if (NUM_ACTUATORS <= pInputBuffer->nActuatorIndex) {
			DbgOut((KERN_ERR "tspdrv: invalid actuator index.\n"));
			i += (SPI_HEADER_SIZE + pInputBuffer->nBufferSize);
			continue;
		}

		if (0 == g_SamplesBuffer[pInputBuffer->nActuatorIndex].actuatorSamples[0].nBufferSize)
			nIndexFreeBuffer = 0;
		else if (0 == g_SamplesBuffer[pInputBuffer->nActuatorIndex].actuatorSamples[1].nBufferSize)
			 nIndexFreeBuffer = 1;
		else {
			/* No room to store new samples */
			DbgOut((KERN_ERR "tspdrv: no room to store new samples.\n"));
			return 0;
		}

		/* Store the data in the actuator's free buffer */
		if (0 != copy_from_user(&(g_SamplesBuffer[pInputBuffer->nActuatorIndex].actuatorSamples[nIndexFreeBuffer]), &(buf[i]), (SPI_HEADER_SIZE + pInputBuffer->nBufferSize))) {
			/* Failed to copy all the data, exit */
			DbgOut((KERN_ERR "tspdrv: copy_from_user failed.\n"));
			return 0;
		}

		/*
		if the no buffer is playing,
		prepare to play g_SamplesBuffer[pInputBuffer->nActuatorIndex].actuatorSamples[nIndexFreeBuffer]
		*/
		if ( -1 == g_SamplesBuffer[pInputBuffer->nActuatorIndex].nIndexPlayingBuffer) {
		 g_SamplesBuffer[pInputBuffer->nActuatorIndex].nIndexPlayingBuffer = nIndexFreeBuffer;
		 g_SamplesBuffer[pInputBuffer->nActuatorIndex].nIndexOutputValue = 0;
		}

		/* Call SPI */
		ImmVibeSPI_ForceOut_SetSamples(pInputBuffer->nActuatorIndex, pInputBuffer->nBitDepth, pInputBuffer->nBufferSize,
			&(g_SamplesBuffer[pInputBuffer->nActuatorIndex].actuatorSamples[nIndexFreeBuffer].dataBuffer[0]));

		/* Increment buffer index */
		i += (SPI_HEADER_SIZE + pInputBuffer->nBufferSize);
	}

#ifdef QA_TEST
	g_nForceLog[g_nForceLogIndex++] = g_cSPIBuffer[0];
	if (g_nForceLogIndex >= FORCE_LOG_BUFFER_SIZE) {
		for (i = 0; i < FORCE_LOG_BUFFER_SIZE; i++) {
			printk("<6>%d\t%d\n", g_nTime, g_nForceLog[i]);
			g_nTime += TIME_INCREMENT;
		}
		g_nForceLogIndex = 0;
	}
#endif

	/* Start the timer after receiving new output force */
	g_bIsPlaying = true;
	VibeOSKernelLinuxStartTimer();

	return count;
}

static long unlocked_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg)
{
#ifdef QA_TEST
	int i;
#endif

	switch (cmd) {
		case TSPDRV_STOP_KERNEL_TIMER:
			/*
			** As we send one sample ahead of time, we need to finish playing the last sample
			** before stopping the timer. So we just set a flag here.
			*/
			if (true == g_bIsPlaying) g_bStopRequested = true;

#ifdef QA_TEST
			if (g_nForceLogIndex) {
				for (i=0; i<g_nForceLogIndex; i++) {
					printk("<6>%d\t%d\n", g_nTime, g_nForceLog[i]);
					g_nTime += TIME_INCREMENT;
				}
			}
			g_nTime = 0;
			g_nForceLogIndex = 0;
#endif
			break;

		case TSPDRV_IDENTIFY_CALLER:
			if (TSPDRV_MAGIC_NUMBER == arg)
				file->private_data = (void*)TSPDRV_MAGIC_NUMBER;
			break;

		case TSPDRV_ENABLE_AMP:
			wake_lock(&vib_wake_lock);
			ImmVibeSPI_ForceOut_AmpEnable(arg);
			break;

		case TSPDRV_DISABLE_AMP:
			ImmVibeSPI_ForceOut_AmpDisable(arg);
			wake_unlock(&vib_wake_lock);
			break;

		case TSPDRV_GET_NUM_ACTUATORS:
			return NUM_ACTUATORS;
	}

	return 0;
}

static int suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret;

	if (g_bIsPlaying)
		ret = -EBUSY;
	else /* Disable system timers */
		ret = 0;

	DbgOut((KERN_DEBUG "tspdrv: %s (%d).\n", __func__, ret));
	return ret;
}

static int resume(struct platform_device *pdev)
{
    DbgOut((KERN_DEBUG "tspdrv: resume.\n"));

	return 0;   /* can resume */
}

static void platform_release(struct device *dev)
{
    DbgOut((KERN_DEBUG "tspdrv: platform_release.\n"));
}
