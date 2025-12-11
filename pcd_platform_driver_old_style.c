#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>

//My platform header
#include "platform.h"

#define MAX_DEVICES 10

struct pcdev_private_data
{
    struct pcdev_platform_data pdata; // Platform data
    char *buffer;                     // Memory buffer
    dev_t dev_num;                    // Device number
    struct cdev cdev;                 // Character device structure
};

struct pcdrv_private_data
{
    int total_devices;
    dev_t device_num_base;
    struct class *pcd_class;
    strut device *pcd_device;
};

struct pcdrv_private_data pcdrv_data;

struct file_operations pcd_fops = {
    .owner = THIS_MODULE,
    // Additional file operations can be added here
};

static int pcd_platform_driver_probe(struct platform_device *pdev)
{
    /*
        Bên device có 2 device trùng tên "pseudo-char-device"
        Nên khi driver được load, hàm probe sẽ được gọi 2 lần, mỗi lần với 1 device khác nhau
        Ta có thể dùng pdev->id để phân biệt 2 device này
    */
    pr_info("PCD Platform Driver Probe Invoked\n");
    // Device initialization code goes here
    return 0;
}

static int pcd_platform_driver_remove(struct platform_device *pdev)
{
    pr_info("PCD Platform Driver Remove Invoked\n");
    // Device cleanup code goes here
    return 0;
}

struct platform_driver pcd_platform_driver = {
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove,
    .driver = {
        .name = "pseudo-char-device", // chỗ này phải trùng với tên device trong pcd_device_setup.c
        .owner = THIS_MODULE,
    },
    // Additional driver operations can be added here
};

static int __init pcd_platform_driver_init(void)
{
    ret = alloc_chrdev_region(&pcdrv_data.device_num_base, 0, MAX_DEVICES, "pcd_devices");
    if (ret < 0)
    {
        pr_err("Failed to allocate chr device\n");
        return ret;
    }

    pcdrv_data.pcd_class = class_create(THIS_MODULE, "pcd_class");
    if (IS_ERR(pcdrv_data.pcd_class))
    {
        pr_err("Class creation failed\n");
        unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
        return PTR_ERR(pcdrv_data.pcd_class);
    }
    

    platform_driver_register(&pcd_platform_driver);
    pr_info("PCD Platform Driver Init\n");
    return 0;
}

static void __exit pcd_platform_driver_exit(void)
{
    platform_driver_unregister(&pcd_platform_driver);

    class_destroy(pcdrv_data.pcd_class);
    unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
    pr_info("PCD Platform Driver Exit\n");
}