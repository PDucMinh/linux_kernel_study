#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"

struct pcdev_platform_data pcdev_pdata[2] = 
{
    [0] = {.size = 512, .perm = RDWR, .serial_number = "PCDEV001"},
    [1] = {.size = 1024, .perm = RDONLY, .serial_number = "PCDEV002"},
};

void pcdev_release(struct device *dev)
{
    pr_info("Device released\n");
}

struct platform_device pcdev_1 
{
    .name = "pseudo-char-device", // chỗ này phải trùng với tên driver trong pcd_platform_driver.c
    .id = 0,
    .dev = {
        .platform_data = &pcdev_pdata[0],
        .release = pcdev_release,
    },
};

struct platform_device pcdev_2
{
    .name = "pseudo-char-device", // chỗ này phải trùng với tên driver trong pcd_platform_driver.c
    .id = 1,
    .dev = {
        .platform_data = &pcdev_pdata[1],
        .release = pcdev_release,
    },
};


static int __init pcdev_init(void)
{
    platform_device_register(&pcdev_1);
    platform_device_register(&pcdev_2);

    pr_info("Platform devices inserted\n");
    return 0;
}

static void __exit pcdev_exit(void)
{
    platform_device_unregister(&pcdev_1);
    platform_device_unregister(&pcdev_2);

    pr_info("Platform devices removed\n");
}

module_init(pcdev_init);
module_exit(pcdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Minh hehe");
MODULE_DESCRIPTION("Platform Device Setup for Pseudo Character Device");

