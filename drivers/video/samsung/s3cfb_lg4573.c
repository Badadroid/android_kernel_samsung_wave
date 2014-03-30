/*
 * LG4573 TFT-LCD Panel Driver for the Samsung Universal board
 *
 * Derived from drivers/video/samsung/s3cfb_hx8369.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/lcd.h>
#include <linux/backlight.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-fb.h>
#include <linux/earlysuspend.h>
#include <linux/pwm.h>
#include <mach/gpio-wave.h>
#include <linux/lg4573.h>


#if defined(CONFIG_FB_S3C_MDNIE)
extern void init_mdnie_class(void);
#endif

#define MAX_BL	255

int bl_freq_count = 100000;

extern int get_lcdtype(void);

struct s5p_lcd {
	int ldi_enabled;
	int bl;
	struct mutex	lock;
	struct device *dev;
	struct spi_device *g_spi;
	struct s5p_lg4573_panel_data	*data;
	struct backlight_device *bl_dev;
	struct lcd_device *lcd_dev;
	struct pwm_device *backlight_pwm_dev;
	struct class *ldi_class;
	struct device *ldi_dev;
	int	lcd_type;
	struct early_suspend    early_suspend;
};


static void update_brightness(struct s5p_lcd *lcd);

static int lg4573_spi_write_data(struct s5p_lcd *lcd, u16 data)
{
	int ret;
	struct spi_message msg;

	struct spi_transfer xfer = {
		.len	= 2,
		.tx_buf	= &data,
	};

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(lcd->g_spi, &msg);

	if (ret < 0)
		pr_err("%s: error: spi_sync (%d)", __func__, ret);

	return ret;
}

static void lg4573_panel_send_sequence(struct s5p_lcd *lcd, const u16 *wbuf)
{
	u8 i = 0;
	u8 addr;

	while ((wbuf[i] & DEFMASK) != ENDDEF)
		if ((wbuf[i] & DEFMASK) != SLEEPMSEC)
		{
			if (lcd->lcd_type == 0 || lcd->lcd_type == 3)
			{
				if ((wbuf[i] & DEFMASK) != DATAMASK)
				{
					addr = 0x74;
				}
				else
				{
					addr = 0x76;
				}
				if(wbuf[i] != 0)
					lg4573_spi_write_data(lcd, addr | ((wbuf[i] & 0xFF) << 8));
			}
			else {
				lg4573_spi_write_data(lcd, wbuf[i]);
			}
			i += 1;
		}
		else
		{
			msleep(wbuf[i+1]);
			i += 2;
		}

}

static ssize_t update_brightness_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct s5p_lcd *lcd = dev_get_drvdata(dev);

	dev_dbg(dev, "called %s\n", __func__);
	return sprintf(buf, "%u\n", lcd->bl);
}

static ssize_t update_brightness_cmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct s5p_lcd *lcd = dev_get_drvdata(dev);
	int brightness = 0;

	sscanf(buf, "%d", &brightness);

	/* Sanity check */
	if(brightness < 0)
		brightness = 0;

	if(brightness > lcd->bl_dev->props.max_brightness)
		brightness = lcd->bl_dev->props.max_brightness;

	lcd->bl = brightness;
	update_brightness(lcd);
	return 0;
}
static DEVICE_ATTR(update_brightness_cmd, 0664, update_brightness_cmd_show, update_brightness_cmd_store);

static void update_brightness(struct s5p_lcd *lcd)
{
	int brightness = lcd->bl;
	if(!lcd->ldi_enabled)
		brightness = 0;

	if(brightness <= 0) {
		s3c_gpio_cfgpin(GPIO_LCD_BL_PWM, S3C_GPIO_OUTPUT);
		gpio_set_value(GPIO_LCD_BL_PWM, 0);
		pwm_disable(lcd->backlight_pwm_dev);
		return;
	}
	s3c_gpio_cfgpin(GPIO_LCD_BL_PWM, 0x2); //PWM output

	pwm_config(lcd->backlight_pwm_dev, (bl_freq_count * brightness)/MAX_BL, bl_freq_count);
	pwm_enable(lcd->backlight_pwm_dev);
}


static void lg4573_ldi_enable(struct s5p_lcd *lcd)
{
	struct s5p_lg4573_panel_data *pdata = lcd->data;
	dev_dbg(lcd->ldi_dev, "%s\n", __func__);
	mutex_lock(&lcd->lock);
	if(lcd->ldi_enabled)
	{
		printk("%s already enabled!\n", __func__);
		goto finito;
	}

	switch (lcd->lcd_type)
	{
		case 1:
			printk(KERN_ERR "%s: Unsupported LCD type 1!\n", __func__);
			break;
		case 2:
			lg4573_panel_send_sequence(lcd, pdata->seq_settings_type2);
			break;
		case 3:
			lg4573_panel_send_sequence(lcd, pdata->seq_settings_type3);
			break;
		case 0:
		default:
			lg4573_panel_send_sequence(lcd, pdata->seq_settings_type0);
			break;
	}

	//lg4573_panel_send_sequence(lcd, pdata->seq_standby_off);

	lcd->ldi_enabled = 1;
	/* Will bring back up previous backlight state with ldi_enabled == 1 */
	update_brightness(lcd);
finito:
	mutex_unlock(&lcd->lock);
}

static void lg4573_ldi_disable(struct s5p_lcd *lcd)
{
	struct s5p_lg4573_panel_data *pdata = lcd->data;

	dev_dbg(lcd->ldi_dev, "%s\n", __func__);
	mutex_lock(&lcd->lock);
	if(!lcd->ldi_enabled)
	{
		printk("%s already disabled!\n", __func__);
		goto finito;
	}
	lg4573_panel_send_sequence(lcd, pdata->seq_standby_on);

	lcd->ldi_enabled = 0;
	/* Will turn off backlight with ldi_enabled == 0 */
	update_brightness(lcd);
finito:
	mutex_unlock(&lcd->lock);
}

static int s5p_lcd_set_power(struct lcd_device *ld, int power)
{
	struct s5p_lcd *lcd = lcd_get_data(ld);

	if (power)
		lg4573_ldi_enable(lcd);
	else
		lg4573_ldi_disable(lcd);

	return 0;
}

static int s5p_lcd_check_fb(struct lcd_device *lcddev, struct fb_info *fi)
{
	return 0;
}

struct lcd_ops s5p_lcd_ops = {
	.set_power = s5p_lcd_set_power,
	.check_fb = s5p_lcd_check_fb,
};

static int s5p_bl_update_status(struct backlight_device *bd)
{
	struct s5p_lcd *lcd = bl_get_data(bd);
	int bl = bd->props.brightness;


	if (bl < 0 || bl > MAX_BL)
		return -EINVAL;

	mutex_lock(&lcd->lock);

	lcd->bl = bl;

	update_brightness(lcd);

	mutex_unlock(&lcd->lock);

	return 0;
}

static int s5p_bl_get_brightness(struct backlight_device *bd)
{
	struct s5p_lcd *lcd = bl_get_data(bd);

	printk(KERN_DEBUG "\n reading brightness\n");

	return lcd->bl;
}

static const struct backlight_ops s5p_bl_ops = {
	.update_status = s5p_bl_update_status,
	.get_brightness = s5p_bl_get_brightness,
};

void lg4573_early_suspend(struct early_suspend *h)
{
	struct s5p_lcd *lcd = container_of(h, struct s5p_lcd, early_suspend);

	lg4573_ldi_disable(lcd);

	return;
}

void lg4573_late_resume(struct early_suspend *h)
{
	struct s5p_lcd *lcd = container_of(h, struct s5p_lcd, early_suspend);

	lg4573_ldi_enable(lcd);

	return;
}

static int __devinit lg4573_probe(struct spi_device *spi)
{
	struct s5p_lcd *lcd;
	int ret;

	lcd = kzalloc(sizeof(struct s5p_lcd), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	mutex_init(&lcd->lock);

	lcd->g_spi = spi;
	lcd->dev = &spi->dev;
	lcd->bl = 128; //half of max brightness

	if (!spi->dev.platform_data) {
		dev_err(lcd->dev, "failed to get platform data\n");
		ret = -EINVAL;
		goto err_setup;
	}
	lcd->data = (struct s5p_lg4573_panel_data *)spi->dev.platform_data;

	if (!lcd->data->seq_settings_type0 || !lcd->data->seq_settings_type1 ||
		!lcd->data->seq_settings_type2 || !lcd->data->seq_settings_type3 ||
		!lcd->data->seq_standby_on || !lcd->data->seq_standby_off ||
		!lcd->data->get_lcdtype) {
		dev_err(lcd->dev, "Invalid platform data\n");
		ret = -EINVAL;
		goto err_setup;
	}

	//determine the LCD type
	lcd->lcd_type = lcd->data->get_lcdtype();

    dev_info(lcd->dev, "LCDTYPE=%d\n", lcd->lcd_type);
	switch(lcd->lcd_type)
	{
		case 1:
		case 2:
			spi->bits_per_word = 9;
			break;
		case 0:
		case 3:
		default:
			spi->bits_per_word = 8;
			break;
	}

	if (spi_setup(spi)) {
		pr_err("failed to setup spi\n");
		ret = -EINVAL;
		goto err_setup;
	}

	ret = gpio_request(GPIO_LCD_BL_PWM, "lcd_bl_pwm");
	if (ret < 0) {
		dev_err(lcd->dev, "unable to request gpio for backlight\n");
		return ret;
	}

	lcd->backlight_pwm_dev = pwm_request(0, "backlight-pwm");

	if (IS_ERR(lcd->backlight_pwm_dev)) {
		dev_err(lcd->dev, "unable to request PWM for backlight\n");
	} else
		dev_err(lcd->dev, "got pwm for backlight\n");


	lcd->bl_dev = backlight_device_register("s5p_bl",
			&spi->dev, lcd, &s5p_bl_ops, NULL);

	if (!lcd->bl_dev) {
		dev_err(lcd->dev, "failed to register backlight\n");
		ret = -EINVAL;
		goto err_setup;
	}

	lcd->bl_dev->props.max_brightness = MAX_BL;

	lcd->lcd_dev = lcd_device_register("s5p_lcd", &spi->dev, lcd, &s5p_lcd_ops);
	if (!lcd->lcd_dev)
	{
		dev_err(lcd->dev, "failed to register lcd\n");
		ret = -EINVAL;
		goto err_setup_lcd;
	}

	// Class and device file creation
	printk(KERN_ERR "ldi_class create\n");

	lcd->ldi_class = class_create(THIS_MODULE, "ldi_class");
	if (IS_ERR(lcd->ldi_class))
		pr_err("Failed to create class(ldi_class)!\n");

	lcd->ldi_dev = device_create(lcd->ldi_class, &spi->dev, 0, lcd, "ldi_dev");
	if (IS_ERR(lcd->ldi_dev))
		pr_err("Failed to create device(ldi_dev)!\n");

	if (!lcd->ldi_dev) {
		dev_err(lcd->dev, "failed to register device(ldi_dev)\n");
		ret = -EINVAL;
		goto err_setup_ldi;
	}

	if (device_create_file(lcd->ldi_dev, &dev_attr_update_brightness_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_update_brightness_cmd.attr.name);

	spi_set_drvdata(spi, lcd);

	lg4573_ldi_enable(lcd);

#ifdef CONFIG_FB_S3C_MDNIE
	init_mdnie_class();  //set mDNIe UI mode, Outdoormode
#endif


#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->early_suspend.suspend = lg4573_early_suspend;
	lcd->early_suspend.resume = lg4573_late_resume;
	lcd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&lcd->early_suspend);
#endif
	dev_dbg(lcd->ldi_dev, "%s successfully probed\n", __func__);

	return 0;

err_setup_ldi:
	lcd_device_unregister(lcd->lcd_dev);

err_setup_lcd:
	backlight_device_unregister(lcd->bl_dev);

err_setup:
	mutex_destroy(&lcd->lock);
	kfree(lcd);

err_alloc:
	return ret;
}


static int __devexit lg4573_remove(struct spi_device *spi)
{
	struct s5p_lcd *lcd = spi_get_drvdata(spi);

	unregister_early_suspend(&lcd->early_suspend);
	backlight_device_unregister(lcd->bl_dev);
	lg4573_ldi_disable(lcd);
	kfree(lcd);

	return 0;
}

static struct spi_driver lg4573_driver = {
	.driver = {
		.name	= "lg4573",
		.owner	= THIS_MODULE,
	},
	.probe		= lg4573_probe,
	.remove		= __devexit_p(lg4573_remove),
};

static int __init lg4573_init(void)
{
	return spi_register_driver(&lg4573_driver);
}
static void __exit lg4573_exit(void)
{
	spi_unregister_driver(&lg4573_driver);
}

module_init(lg4573_init);
module_exit(lg4573_exit);


MODULE_AUTHOR("Oleg Kiya");
MODULE_DESCRIPTION("LG4573 LDI driver");
MODULE_LICENSE("GPL");
