#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/input.h>
#include <linux/gpio.h>

#include <mach/gpio.h>

#define GREEN_LED 0
#define RED_LED 1

static struct {
	int gpio_index; // Control index
	int gpio; // GPIO number
	char *name; // GPIO name
	bool output; // 1= output, 0=input
	int value; // Default (only for output)
	int pud; // Pull up/down register setting
} gonzo_gpio_map[] = {
	{ GREEN_LED, 32, "green_led", 1, 0, 0 },
	{ RED_LED, 36, "red_led", 1, 0, 0 },
};

/* Setup sysfs */
static ssize_t show_gpio(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t set_gpio(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static DEVICE_ATTR(green_led, S_IRWXUGO, show_gpio, set_gpio);
static DEVICE_ATTR(red_led, S_IRWXUGO, show_gpio, set_gpio);

static struct attribute *gonzo_sysfs_entries[] = {
	&dev_attr_green_led.attr,
	&dev_attr_red_led.attr,
	NULL
};

static struct attribute_group gonzo_sysfs_attr_group = {
	.name = NULL,
	.attrs = gonzo_sysfs_entries,
};


static ssize_t show_gpio(struct device *dev, struct device_attribute *attr, char *buf) 
{
	int i;
	for (i = 0; i < ARRAY_SIZE(gonzo_gpio_map); i++) {
		if (gonzo_gpio_map[i].gpio && (strcmp(gonzo_gpio_map[i].name, attr->attr.name))) {
			return sprintf(buf, "%d\n", gpio_get_value(gonzo_gpio_map[i].gpio) ? 1 : 0);
		}
	}

	return sprintf(buf, "ERROR: GPIO specified not found\n");
}

static ssize_t set_gpio(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int val, i;

	if (!(sscanf(buf, "%d\n", &val))) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(gonzo_gpio_map); i++) {
		if (gonzo_gpio_map[i].gpio && (strcmp(gonzo_gpio_map[i].name, attr->attr.name))) {
			if (gonzo_gpio_map[i].output) {
				gpio_set_value(gonzo_gpio_map[i].gpio, (val != 0) ? 1 : 0);
			} else {
				printk("This GPIO is configured for input, no value being set\n");
			}
		}
	}

	printk("ERROR: GPIO not found\n");
	return count;
}


/* Wire up the actual sysfs calls */

static int gonzo_sysfs_probe(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(gonzo_gpio_map); i++) {
		if (gonzo_gpio_map[i].gpio) {
			if (gpio_request(gonzo_gpio_map[i].gpio, gonzo_gpio_map[i].name)) {
				printk("%s : %s gpio_request error!\n", __FUNCTION__, gonzo_gpio_map[i].name);
				continue;
			}

			if (gonzo_gpio_map[i].output) {
				gpio_direction_output(gonzo_gpio_map[i].gpio, gonzo_gpio_map[i].value);
			} else {
				gpio_direction_input(gonzo_gpio_map[i].gpio);
			}
			//setpull, if possible
		}
	}

	return sysfs_create_group(&pdev->dev.kobj, &gonzo_sysfs_attr_group);
}

static int gonzo_sysfs_remove(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(gonzo_gpio_map); i++) {
		if (gonzo_gpio_map[i].gpio) {
			gpio_free(gonzo_gpio_map[i].gpio);
		}
	}

	sysfs_remove_group(&pdev->dev.kobj, &gonzo_sysfs_attr_group);
	return 0;
}

static struct platform_driver gonzo_sysfs_driver = {
	.driver = {
		.name = "gonzo-sysfs",
		.owner = THIS_MODULE,
	},
	.probe = gonzo_sysfs_probe,
	.remove = gonzo_sysfs_remove,
};

static int __init gonzo_sysfs_init(void)
{
	return platform_driver_register(&gonzo_sysfs_driver);
}

static void __exit gonzo_sysfs_exit(void)
{
	return platform_driver_unregister(&gonzo_sysfs_driver);
}


module_init(gonzo_sysfs_init);
module_exit(gonzo_sysfs_exit);


MODULE_DESCRIPTION("SYSFS driver for LEDs on Gonzo");
MODULE_AUTHOR("Telenor Digital AS");
MODULE_LICENSE("GPL");
