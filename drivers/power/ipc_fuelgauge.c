/*
 *  ipc_fuelgauge.c
 *  Driver allowing charger driver to know battery state, updated by RIL
 *
 *  Based on s3c_fake_battery.c and max17040_battery.c
 *
 *  Copyright (C) 2012 Dominik Marszk <dmarszk@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/time.h>

#include <linux/ipc_fuelgauge.h>

#define DRIVER_NAME	"ipc-fuelgauge"

struct ipc_fuelgauge {
	struct power_supply		battery;
	struct ipc_fuelgauge_platform_data	*pdata;

	/* State Of Connect */
	int online;
	/* battery voltage */
	int vcell;
	/* battery capacity */
	int soc;
	/* State Of Charge */
	int status;
};
static ssize_t ipc_fuelgauge_attr_show(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf);
static ssize_t ipc_fuelgauge_attr_store(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count);
				 
static int ipc_fuelgauge_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct ipc_fuelgauge *chip = container_of(psy,
				struct ipc_fuelgauge, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = chip->status;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chip->online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chip->vcell * 1250;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = chip->soc;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#define SEC_BATTERY_ATTR(_name)								\
{											\
	.attr = { .name = #_name, .mode = S_IRUGO | S_IWUGO/*, .owner = THIS_MODULE */},	\
	.show = ipc_fuelgauge_attr_show,							\
	.store = ipc_fuelgauge_attr_store,								\
}

static struct device_attribute ipc_fuelgauge_attrs[] = {
	SEC_BATTERY_ATTR(batt_vcell),
	SEC_BATTERY_ATTR(batt_soc),
	SEC_BATTERY_ATTR(batt_status),
	SEC_BATTERY_ATTR(batt_online),
};

enum {
	BATT_VCELL = 0,
	BATT_SOC,
	BATT_STATUS,
	BATT_ONLINE,
};
				 
static ssize_t ipc_fuelgauge_attr_show(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf)
{
	int i = 0;
	const ptrdiff_t off = attr - ipc_fuelgauge_attrs;
	struct platform_device *pdev = to_platform_device(dev);
	struct ipc_fuelgauge *chip = platform_get_drvdata(pdev);

	switch (off) {
	case BATT_VCELL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
						chip->vcell);
	break;
	case BATT_SOC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
						chip->soc);
		break;
	case BATT_STATUS:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
						chip->status);
		break;
	case BATT_ONLINE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
						chip->online);
		break;
	default:
		i = -EINVAL;
	}       

	return i;
}

static ssize_t ipc_fuelgauge_attr_store(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int x = 0;
	int ret = 0;
	const ptrdiff_t off = attr - ipc_fuelgauge_attrs;
	struct platform_device *pdev = to_platform_device(dev);
	struct ipc_fuelgauge *chip = platform_get_drvdata(pdev);

	switch (off) {
	case BATT_VCELL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			chip->vcell = x;
			ret = count;
		}
		dev_info(dev, "%s : vcell = %d\n", __func__, x);
		break;
	case BATT_SOC:
		if (sscanf(buf, "%d\n", &x) == 1) {
			chip->soc = x;
			ret = count;
		}
		dev_info(dev, "%s : soc = %d\n", __func__, x);
		break;	
	case BATT_STATUS:
		if (sscanf(buf, "%d\n", &x) == 1) {
			chip->status = x;
			ret = count;
		}
		dev_info(dev, "%s : status = %d\n", __func__, x);
		break;	
	case BATT_ONLINE:
		if (sscanf(buf, "%d\n", &x) == 1) {
			chip->online = x;
			ret = count;
		}
		dev_info(dev, "%s : online = %d\n", __func__, x);
		break;
	default:
		ret = -EINVAL;
	}       

	return ret;
}

static enum power_supply_property ipc_fuelgauge_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
};

static int __devinit ipc_fuelgauge_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct ipc_fuelgauge *chip;
	
	struct device* dev = &pdev->dev;
	dev_info(dev, "%s\n", __func__);

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;
		
	chip->status = POWER_SUPPLY_STATUS_UNKNOWN;
	
	chip->pdata = dev->platform_data;

	chip->battery.name		= "battery";
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->battery.get_property	= ipc_fuelgauge_get_property;
	chip->battery.properties	= ipc_fuelgauge_battery_props;
	chip->battery.num_properties	= ARRAY_SIZE(ipc_fuelgauge_battery_props);

	if (chip->pdata && chip->pdata->power_supply_register)
		ret = chip->pdata->power_supply_register(dev, &chip->battery);
	else
		ret = power_supply_register(dev, &chip->battery);
	if (ret) {
		dev_err(dev, "failed: power supply register\n");
		kfree(chip);
		return ret;
	}

	platform_set_drvdata(pdev, chip);

	return ret;
}

static int __devexit ipc_fuelgauge_remove(struct platform_device *pdev)
{
	struct ipc_fuelgauge *chip = platform_get_drvdata(pdev);
	struct device* dev = &pdev->dev;
	dev_info(dev, "%s\n", __func__);
	
	if (chip->pdata && chip->pdata->power_supply_unregister)
		chip->pdata->power_supply_unregister(&chip->battery);
	else
		power_supply_unregister(&chip->battery);
	kfree(chip);
 
	return 0;
}

static struct platform_driver ipc_fuelgauge_driver = {
	.driver.name	= DRIVER_NAME,
	.driver.owner	= THIS_MODULE,
	.probe		= ipc_fuelgauge_probe,
	.remove		= __devexit_p(ipc_fuelgauge_remove),
};

static int __init ipc_fuelgauge_init(void)
{	
	return platform_driver_register(&ipc_fuelgauge_driver);
}
module_init(ipc_fuelgauge_init);

static void __exit ipc_fuelgauge_exit(void)
{
	platform_driver_unregister(&ipc_fuelgauge_driver);
}
module_exit(ipc_fuelgauge_exit);

MODULE_AUTHOR("Dominik Marszk <dmarszk@gmail.com>");
MODULE_DESCRIPTION("IPC Fuel Gauge Interface");
MODULE_LICENSE("GPL");
