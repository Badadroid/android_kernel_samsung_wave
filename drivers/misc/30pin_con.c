/*
 * Copyright (C) 2008 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/30pin_con.h>
#include <asm/irq.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
#include <mach/gpio-p1.h>

#define IRQ_ACCESSORY_INT    IRQ_EINT5
#define IRQ_DOCK_INT         IRQ_EINT(29)
#define ACCESSORY_ID         4

extern struct i2c_driver SII9234_i2c_driver;
extern struct i2c_driver SII9234A_i2c_driver;
extern struct i2c_driver SII9234B_i2c_driver;
extern struct i2c_driver SII9234C_i2c_driver;

extern int check_keyboard_dock(int val);
extern void TVout_LDO_ctrl(int enable);
extern void sii9234_tpi_init(void);
extern void MHD_HW_Off(void);
extern int MHD_HW_IsOn(void);
extern int MHD_Read_deviceID(void);
extern void MHD_GPIO_INIT(void);
extern int s3c_adc_get_adc_data(int channel);

#if defined CONFIG_USB_S3C_OTG_HOST || defined CONFIG_USB_DWC_OTG
extern void set_otghost_mode(int mode);
#endif

bool enable_audio_usb = false;

struct acc_con_info {
	struct device *acc_dev;
	struct wake_lock wake_lock;
	struct work_struct dwork;
	struct work_struct awork;
	struct workqueue_struct *con_workqueue;
	struct workqueue_struct *id_workqueue;
	enum accessory_type current_accessory;
	enum dock_type current_dock;
	int dock_state;
	int acc_state;
};

static ssize_t MHD_check_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int count;
	int res;
	TVout_LDO_ctrl(true);
	if (!MHD_HW_IsOn()) {
		sii9234_tpi_init();
		res = MHD_Read_deviceID();
		MHD_HW_Off();		
	} else {
		sii9234_tpi_init();
		res = MHD_Read_deviceID();
	}
	count = sprintf(buf,"%d\n", res);
	TVout_LDO_ctrl(false);
	return count;
}

static ssize_t MHD_check_write(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("[30pin] input data: %s\n", buf);
	return size;
}

static DEVICE_ATTR(MHD_file, S_IRUGO , MHD_check_read, MHD_check_write);

static ssize_t acc_check_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int count;
	int connected = 0;
	struct acc_con_info *acc = dev_get_drvdata(dev);

	if (0 == acc->dock_state) {
		if(acc->current_dock == DOCK_DESK)
			connected |= (0x1 << 0);
		else if (acc->current_dock == DOCK_KEYBOARD)
			connected |= (0x1 << 1);
	}

	if (0 == acc->acc_state) {
		if (acc->current_accessory == ACCESSORY_CARMOUNT)
			connected |= (0x1 << 2);
		else if (acc->current_accessory == ACCESSORY_TVOUT)
			connected |= (0x1 << 3);
		else if (acc->current_accessory == ACCESSORY_LINEOUT)
			connected |= (0x1 << 4);
	}

	if (gpio_get_value(GPIO_HDMI_HPD) && MHD_HW_IsOn())
		connected |= (0x1 << 5);

	count = sprintf(buf,"%d\n", connected);
	pr_info("[30pin] connected: %x\n", connected);
	return count;
}

static ssize_t acc_check_write(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("[30pin] input data: %s\n", buf);
	return size;
}

static DEVICE_ATTR(acc_file, S_IRUGO , acc_check_read, acc_check_write);

static int connector_detect_change(void)
{
	int i;
	s16 adc_sum = 0;
	s16 adc_buff[5];
	s16 mili_volt;
	s16 adc_min = 0;
	s16 adc_max = 0;
	int adc_res = 0;

	for (i = 0; i < 5; i++) {
		/*change this reading ADC function  */
		mili_volt = s3c_adc_get_adc_data(ACCESSORY_ID);
		adc_buff[i] = mili_volt;
		adc_sum += adc_buff[i];
		if (i == 0) {
			adc_min = adc_buff[0];
			adc_max = adc_buff[0];
		} else {
			if (adc_max < adc_buff[i])
				adc_max = adc_buff[i];
			else if (adc_min > adc_buff[i])
				adc_min = adc_buff[i];
		}
		msleep(20);
	}
	adc_res = (adc_sum - adc_max - adc_min) / 3;
	return adc_res;
}

static void _detected(struct acc_con_info *acc, int device, bool connected)
{

	enable_audio_usb = false;

	if (connected) {
		switch(device) {
#if defined(CONFIG_USB_S3C_OTG_HOST) || defined(CONFIG_USB_DWC_OTG)
		case P30_OTG:
			pr_info("[30pin] OTG cable detected: id=%d\n", device);
			set_otghost_mode(2);
			break;
#endif
		case P30_EARJACK_WITH_DOCK:
			pr_info("[30pin] Earjack with Dock detected: id=%d\n", device);
			enable_audio_usb = true;
			break;
		case P30_ANAL_TV_OUT:
			pr_info("[30pin] TVOut cable detected: id=%d\n", device);
			TVout_LDO_ctrl(true);
			enable_audio_usb = true;
			break;
		case P30_KEYBOARDDOCK:
			pr_info("[30pin] Keyboard dock detected: id=%d\n", device);
			acc->current_dock = DOCK_KEYBOARD;
			break;
		case P30_CARDOCK:
			pr_info("[30pin] Car dock detected: id=%d\n", device);
			//to do
			break;
		case P30_DESKDOCK:
			pr_info("[30pin] Deskdock cable detected: id=%d\n", device);
			acc->current_dock = DOCK_DESK;
			TVout_LDO_ctrl(true);
			sii9234_tpi_init();
			break;
		}
	} else {
		switch(device) {
#if defined(CONFIG_USB_S3C_OTG_HOST) || defined(CONFIG_USB_DWC_OTG)
		case P30_OTG:
			set_otghost_mode(0);
			break;
#endif
		case P30_EARJACK_WITH_DOCK:
			//to do
			break;
		case P30_ANAL_TV_OUT:
			TVout_LDO_ctrl(false);
			break;
		case P30_KEYBOARDDOCK:
			check_keyboard_dock(1);
			break;
		case P30_DESKDOCK:
			MHD_HW_Off();
			TVout_LDO_ctrl(false);
			break;
		}
	}
}

static void acc_dock_check(struct acc_con_info *acc, bool connected)
{
	char *envp[3];
	char *env_ptr  = "DOCK=none";
	char *stat_ptr = "STATE=offline";

	if (connected)
		stat_ptr = "STATE=online";

	if (acc->current_dock == DOCK_KEYBOARD)
		env_ptr = "DOCK=keyboard";
	else if (acc->current_dock == DOCK_DESK)
		env_ptr = "DOCK=desk";

	pr_info("[30pin] %s: %s - %s\n", __func__, env_ptr, stat_ptr);

	envp[0] = env_ptr;
	envp[1] = stat_ptr;
	envp[2] = NULL;
	kobject_uevent_env(&acc->acc_dev->kobj, KOBJ_CHANGE, envp);
}

static irqreturn_t acc_con_interrupt(int irq, void *ptr)
{
	struct acc_con_info *acc = ptr;
	pr_info("[30pin] %s\n", __func__);
	disable_irq_nosync(IRQ_ACCESSORY_INT);
	queue_work(acc->con_workqueue, &acc->dwork);
	return IRQ_HANDLED;
}

static int acc_con_interrupt_init(struct acc_con_info *acc)
{
	int ret;
	pr_info("[30pin] %s\n", __func__);

	s3c_gpio_cfgpin(GPIO_ACCESSORY_INT, S3C_GPIO_SFN(GPIO_ACCESSORY_INT_AF));
	s3c_gpio_setpull(GPIO_ACCESSORY_INT, S3C_GPIO_PULL_UP);
	irq_set_irq_type(IRQ_ACCESSORY_INT, IRQ_TYPE_EDGE_BOTH);

	ret = request_threaded_irq(IRQ_ACCESSORY_INT, NULL, acc_con_interrupt,
		IRQF_DISABLED, "Docking Detected", acc);

	if (unlikely(ret < 0)) {
		pr_err("[30pin] fail to register irq : GPIO_ACCESSORY_INT\n");
		return ret;
	}

	return 0;
}

static void acc_notified(struct acc_con_info *acc, s16 acc_adc)
{
	char *envp[3];
	char *env_ptr  = "ACCESSORY=unknown";
	char *stat_ptr = "STATE=offline";
	acc->current_accessory = ACCESSORY_NONE;

	pr_info("[30pin] adc change notified: acc_adc = %d\n", acc_adc);

	/*
	 * P30 Standard
	 * -------------------------------------------------------------
	 * Accessory		Vacc [V]		adc
	 * -------------------------------------------------------------
	 * OTG			2.2 (2.1~2.3)		2731 (2606~2855)
	 * Analog TV Cable	1.8 (1.7~1.9)		2234 (2100~2360)
	 * Car Mount		1.38 (1.28~1.48)	1715 (1590~1839)
	 * 3-pole Earjack	0.99 (0.89~1.09)	1232 (1107~1356)
	 * -------------------------------------------------------------
	 */

	if (acc_adc) {
		if ((2600 < acc_adc) && (acc_adc < 2860)) {
			/* Camera Connection Kit */
			env_ptr = "ACCESSORY=OTG";
			acc->current_accessory = ACCESSORY_OTG;
			_detected(acc, P30_OTG, true);
		} else if ((2100 < acc_adc) && (acc_adc < 2360)) {
			/* Analog TV Out Cable */
			env_ptr = "ACCESSORY=TV";
			acc->current_accessory = ACCESSORY_TVOUT;
			_detected(acc, P30_ANAL_TV_OUT, true);
		} else if ((1590 < acc_adc) && (acc_adc < 1840)) {
			/* Car Mount (charge 5V/2A) */
			env_ptr = "ACCESSORY=carmount";
			acc->current_accessory = ACCESSORY_CARMOUNT;
			_detected(acc, P30_CARDOCK, true);
		} else if ((1100 < acc_adc) && (acc_adc < 1360)) {
			/* 3-Pole Ear-Jack with Deskdock*/
			env_ptr = "ACCESSORY=lineout";
			acc->current_accessory = ACCESSORY_LINEOUT;
			_detected(acc, P30_EARJACK_WITH_DOCK, true);
		} else {
			pr_warning("[30pin] adc range filter not found.\n");
			return;
		}

		stat_ptr = "STATE=online";

		if (acc->current_accessory == ACCESSORY_OTG)
			msleep(100);

	} else {
		if (acc->current_accessory == ACCESSORY_OTG) {
			env_ptr = "ACCESSORY=OTG";
			_detected(acc, P30_OTG, false);
		} else if (acc->current_accessory == ACCESSORY_TVOUT) {
			env_ptr = "ACCESSORY=TV";
			_detected(acc, P30_ANAL_TV_OUT, false);
		} else if (acc->current_accessory == ACCESSORY_LINEOUT) {
			env_ptr = "ACCESSORY=lineout";
			_detected(acc, P30_EARJACK_WITH_DOCK, false);
		} else if (acc->current_accessory == ACCESSORY_CARMOUNT) {
			env_ptr = "ACCESSORY=carmount";
			_detected(acc, P30_CARDOCK, false);
		}
	}

	pr_info("[30pin] %s: %s - %s\n", __func__, env_ptr, stat_ptr);

	envp[0] = env_ptr;
	envp[1] = stat_ptr;
	envp[2] = NULL;
	kobject_uevent_env(&acc->acc_dev->kobj, KOBJ_CHANGE, envp);
}

static void acc_con_worker(struct work_struct *work)
{
	int cur_state;
	int delay = 10;
	struct acc_con_info *acc = container_of(work, struct acc_con_info, dwork);

	cur_state = gpio_get_value(GPIO_ACCESSORY_INT);
	pr_info("[30pin] accessory_id irq handler: dock_irq gpio val = %d\n", cur_state);

	while (delay > 0) {
		if (cur_state != gpio_get_value(GPIO_ACCESSORY_INT))
			return;
		usleep_range(10000, 11000);
		delay--;
	}

	if (cur_state != acc->dock_state) {
		if (1 == cur_state) {
			pr_info("[30pin] Docking station detatched");
			acc->dock_state = cur_state;

			if (acc->current_dock == DOCK_KEYBOARD)
				_detected(acc, P30_KEYBOARDDOCK, false);

			if (acc->current_dock == DOCK_DESK)
				_detected(acc, P30_DESKDOCK, false);

			acc->current_dock = DOCK_NONE;
			acc_dock_check(acc, false);
		} else if (0 == cur_state) {
			pr_info("[30pin] Docking station attatched\n");
			acc->dock_state = cur_state;

			if (check_keyboard_dock(cur_state))
				_detected(acc, P30_KEYBOARDDOCK, true);
			else
				_detected(acc, P30_DESKDOCK, true);

			acc_dock_check(acc, true);
		}
	}
	enable_irq(IRQ_ACCESSORY_INT);
}

static void acc_id_worker(struct work_struct *work)
{
	struct acc_con_info *acc = container_of(work, struct acc_con_info, awork);
	int acc_id_val;
	int adc_val = 0;

	acc_id_val = gpio_get_value(GPIO_DOCK_INT);
	pr_info("[30pin] accessorry_id irq handler: dock_irq gpio val = %d\n", acc_id_val);

	if (acc_id_val != acc->acc_state) {
		if (1 == acc_id_val) {
			pr_info("[30pin] Accessory detached");
			acc->acc_state = acc_id_val;
			acc_notified(acc, false);
			irq_set_irq_type(IRQ_DOCK_INT, IRQ_TYPE_EDGE_FALLING);
		} else if (0 == acc_id_val) {
			acc->acc_state = acc_id_val;
			msleep(420); /* workaround for jack */
			wake_lock(&acc->wake_lock);
			adc_val = connector_detect_change();
			pr_info("[30pin] Accessory attached, adc=%d\n", adc_val);

			acc_notified(acc, adc_val);
			irq_set_irq_type(IRQ_DOCK_INT, IRQ_TYPE_EDGE_RISING);
			wake_unlock(&acc->wake_lock);
		}
	}
	enable_irq(IRQ_DOCK_INT);
}

static irqreturn_t acc_id_interrupt(int irq, void *ptr)
{
	struct acc_con_info *acc = ptr;
	pr_info("[30pin] %s\n", __func__);
	disable_irq_nosync(IRQ_DOCK_INT);
	queue_work(acc->id_workqueue, &acc->awork);
	return IRQ_HANDLED;
}

static int acc_id_interrupt_init(struct acc_con_info *acc)
{
	int ret;
	pr_info("[30pin] %s\n", __func__);

	s3c_gpio_cfgpin(GPIO_DOCK_INT, S3C_GPIO_SFN(GPIO_DOCK_INT_AF));
	s3c_gpio_setpull(GPIO_DOCK_INT, S3C_GPIO_PULL_NONE);
	irq_set_irq_type(IRQ_DOCK_INT, IRQ_TYPE_EDGE_BOTH);

	ret = request_threaded_irq(IRQ_DOCK_INT, NULL, acc_id_interrupt,
		IRQF_DISABLED, "Accessory Detected", acc);

	if (unlikely(ret < 0)) {
		pr_err("[30pin] request dock_irq failed.\n");
		return ret;
	}
	return 0;
}

static void initial_connection_check(struct acc_con_info *acc)
{
	int adc_val = 0;

	/* checks dock connectivity before registers dock irq */
	acc->dock_state = gpio_get_value(GPIO_ACCESSORY_INT);

	if (!acc->dock_state)
		acc_dock_check(acc, true);

	/* checks otg connectivity before registers otg irq */
	acc->acc_state = gpio_get_value(GPIO_DOCK_INT);
	if (!acc->acc_state) {
		wake_lock(&acc->wake_lock);
		msleep(420); /* workaround for jack */
		adc_val = connector_detect_change();
		acc_notified(acc, adc_val);
		wake_unlock(&acc->wake_lock);
	}
}

static int acc_con_probe(struct platform_device *pdev)
{
	struct acc_con_info *acc;
	int retval = 0;
	pr_info("[30pin] %s\n", __func__);

	acc = kzalloc(sizeof(*acc), GFP_KERNEL);
	if (!acc)
		return -ENOMEM;

	acc->current_dock      = DOCK_NONE;
	acc->current_accessory = ACCESSORY_NONE;
	dev_set_drvdata(&pdev->dev, acc);
	acc->acc_dev = &pdev->dev;

	retval = i2c_add_driver(&SII9234A_i2c_driver);
	if (retval != 0)
		pr_info("[30pin] MHL SII9234A can't add i2c driver\n");

	retval = i2c_add_driver(&SII9234B_i2c_driver);
	if (retval != 0)
		pr_info("[30pin] MHL SII9234B can't add i2c driver\n");

	retval = i2c_add_driver(&SII9234C_i2c_driver);
	if (retval != 0)
		pr_info("[30pin] MHL SII9234C can't add i2c driver\n");

	retval = i2c_add_driver(&SII9234_i2c_driver);
	if (retval != 0)
		pr_info("[30pin] MHL SII9234 can't add i2c driver\n");

	MHD_GPIO_INIT();
	MHD_HW_Off();

	wake_lock_init(&acc->wake_lock, WAKE_LOCK_SUSPEND, "30pin_con");

	initial_connection_check(acc);
	msleep(200);

	INIT_WORK(&acc->dwork, acc_con_worker);
	acc->con_workqueue = create_singlethread_workqueue("acc_con_workqueue");
	retval = acc_con_interrupt_init(acc);
	if (retval != 0) {
		pr_err("[30pin] acc_con_interrupt_init failed.\n");
		return retval;
	}

	INIT_WORK(&acc->awork, acc_id_worker);
	acc->id_workqueue = create_singlethread_workqueue("acc_id_workqueue");
	retval = acc_id_interrupt_init(acc);
	if (retval != 0) {
		pr_err("[30pin] acc_id_interrupt_init failed.\n");
		return retval;
	}

	if (device_create_file(acc->acc_dev, &dev_attr_MHD_file) < 0)
		pr_err("[30pin] Failed to create device file(%s)!\n",
			dev_attr_MHD_file.attr.name);

	if (device_create_file(acc->acc_dev, &dev_attr_acc_file) < 0)
		pr_err("[30pin] Failed to create device file(%s)!\n",
			dev_attr_acc_file.attr.name);

	retval = enable_irq_wake(IRQ_ACCESSORY_INT);
	if (unlikely(retval < 0)) {
		pr_err("[30pin] enable accessory_irq failed.\n");
		return retval;
	}

	retval = enable_irq_wake(IRQ_DOCK_INT);
	if (unlikely(retval < 0)) {
		pr_err("[30pin]  enable dock_irq failed.\n");
		return retval;
	}

	return retval;
}

static int acc_con_remove(struct platform_device *pdev)
{
	struct acc_con_info *acc = platform_get_drvdata(pdev);
	pr_info("[30pin] %s\n", __func__);

	i2c_del_driver(&SII9234A_i2c_driver);
	i2c_del_driver(&SII9234B_i2c_driver);
	i2c_del_driver(&SII9234C_i2c_driver);
	i2c_del_driver(&SII9234_i2c_driver);

	disable_irq_wake(IRQ_ACCESSORY_INT);
	disable_irq_wake(IRQ_DOCK_INT);
	kfree(acc);

	return 0;
}

#ifdef CONFIG_PM
static int acc_con_suspend(struct device *dev)
{
	pr_info("[30pin] %s\n", __func__);
	MHD_HW_Off();

	return 0;
}

static int acc_con_resume(struct device *dev)
{
	struct acc_con_info *acc = dev_get_drvdata(dev);
	int dock_state;
	pr_info("[30pin] %s\n", __func__);

	dock_state = gpio_get_value(GPIO_ACCESSORY_INT);

	if (!dock_state)
		if (acc->current_dock == DOCK_DESK)
			sii9234_tpi_init();

	return 0;
}

static const struct dev_pm_ops acc_con_pm_ops = {
	.suspend	= acc_con_suspend,
	.resume		= acc_con_resume,
};
#endif

static struct platform_driver acc_con_driver = {
	.probe		= acc_con_probe,
	.remove		= acc_con_remove,
	.driver		= {
		.name		= "acc_con",
		.owner		= THIS_MODULE,
#ifdef CONFIG_PM
		.pm         = &acc_con_pm_ops,
#endif
	},
};

static int __init acc_con_init(void)
{
	pr_info("[30pin] %s\n", __func__);
	return platform_driver_register(&acc_con_driver);
}

static void __exit acc_con_exit(void)
{
	platform_driver_unregister(&acc_con_driver);
}

late_initcall(acc_con_init);
module_exit(acc_con_exit);

MODULE_AUTHOR("Humberto Borba <kernel@humberos.com.br>");
MODULE_DESCRIPTION("acc connector driver");
MODULE_LICENSE("GPL");
