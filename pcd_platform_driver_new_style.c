#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h> // Cần cho of_device_id

// Thay vì define cứng, ta thường lấy từ Device Tree hoặc cấp phát động
#define CLASS_NAME "pcd_class"
#define DRIVER_NAME "pseudo-char-device"

struct pcdev_private_data {
    char *buffer;
    dev_t dev_num;
    struct cdev cdev;
    struct class *pcd_class; // Lưu class vào đây hoặc dùng global static nếu share
    struct device *pcd_device;
    int size;
};

// Hàm open/release/read/write (giữ nguyên logic của bạn, rút gọn để tập trung vào platform)
static int pcd_open(struct inode *inode, struct file *file) {
    // Kỹ thuật hiện đại: Lấy private data từ container_of hoặc inode->i_cdev
    struct pcdev_private_data *pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);
    file->private_data = pcdev_data; 
    pr_info("Device opened\n");
    return 0;
}

static struct file_operations pcd_fops = {
    .owner = THIS_MODULE,
    .open = pcd_open,
    // .read = ..., .write = ...
};

/* * HIỆN ĐẠI 1: Hàm Probe dùng Devres (devm_*)
 * Kernel sẽ TỰ ĐỘNG giải phóng bộ nhớ/tài nguyên khi driver bị remove hoặc probe thất bại.
 */
static int pcd_platform_driver_probe(struct platform_device *pdev)
{
    struct pcdev_private_data *pdata;
    int ret;
    struct device *dev = &pdev->dev; // Lấy struct device để dùng cho devm_

    pr_info("Probing device with DT compatible string\n");

    /* 1. Cấp phát bộ nhớ cho private data bằng devm_kzalloc 
       Không cần kfree trong hàm remove */
    pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
    if (!pdata)
        return -ENOMEM;

    /* 2. Lưu pointer pdata vào platform_device để dùng lại ở các hàm khác (như remove) */
    platform_set_drvdata(pdev, pdata);

    /* 3. Cấp phát dynamic device number (Major/Minor) */
    ret = alloc_chrdev_region(&pdata->dev_num, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        dev_err(dev, "Failed to allocate chrdev region\n");
        return ret;
    }

    /* 4. Tạo class (Lưu ý: class_create có thể thay đổi tùy version kernel, kernel mới bỏ tham số owner) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    pdata->pcd_class = class_create(CLASS_NAME);
#else
    pdata->pcd_class = class_create(THIS_MODULE, CLASS_NAME);
#endif

    if (IS_ERR(pdata->pcd_class)) {
        ret = PTR_ERR(pdata->pcd_class);
        goto unreg_chrdev; // Vẫn cần cleanup thủ công cho những thứ non-devm (như alloc_chrdev_region)
    }

    /* 5. Init và Add Cdev */
    cdev_init(&pdata->cdev, &pcd_fops);
    pdata->cdev.owner = THIS_MODULE;
    ret = cdev_add(&pdata->cdev, pdata->dev_num, 1);
    if (ret < 0) {
        dev_err(dev, "Failed to add cdev\n");
        goto destroy_class;
    }

    /* 6. Tạo device file /dev/xyz 
       Dùng device_create (thay vì mknod thủ công) */
    pdata->pcd_device = device_create(pdata->pcd_class, NULL, pdata->dev_num, NULL, "pcd_dev-%d", pdev->id);
    if (IS_ERR(pdata->pcd_device)) {
        dev_err(dev, "Failed to create device\n");
        ret = PTR_ERR(pdata->pcd_device);
        goto del_cdev;
    }

    pr_info("Device probe successful\n");
    return 0;

    // Error handling path (Vẫn cần thiết cho logic Char Device)
del_cdev:
    cdev_del(&pdata->cdev);
destroy_class:
    class_destroy(pdata->pcd_class);
unreg_chrdev:
    unregister_chrdev_region(pdata->dev_num, 1);
    return ret;
}

/* * Hàm Remove: Cleanup ngược lại với Probe
 * Những thứ dùng devm_ (như pdata) KHÔNG cần free ở đây.
 */
static int pcd_platform_driver_remove(struct platform_device *pdev)
{
    // Lấy lại dữ liệu đã lưu
    struct pcdev_private_data *pdata = platform_get_drvdata(pdev);

    device_destroy(pdata->pcd_class, pdata->dev_num);
    cdev_del(&pdata->cdev);
    class_destroy(pdata->pcd_class);
    unregister_chrdev_region(pdata->dev_num, 1);
    
    pr_info("Device removed\n");
    return 0;
}

/*
 * HIỆN ĐẠI 2: Device Tree Matching
 * Thay vì dùng .name, ta dùng .compatible để khớp với file .dts
 */
static const struct of_device_id pcd_dt_ids[] = {
    { .compatible = "org,pseudo-char-device", }, // Chuỗi này phải có trong file .dts
    { }
};
MODULE_DEVICE_TABLE(of, pcd_dt_ids);

static struct platform_driver pcd_platform_driver = {
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove, // Hàm remove đã sửa signature trong kernel mới (trả về void hoặc int tùy version)
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = pcd_dt_ids, // Liên kết bảng match Device Tree
    },
};

/*
 * HIỆN ĐẠI 3: module_platform_driver()
 * Macro này thay thế hoàn toàn module_init() và module_exit() thủ công
 * Nó tự động gọi platform_driver_register() và unregister()
 */
module_platform_driver(pcd_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Modern Platform Driver Example");