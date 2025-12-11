#include <linux/property.h> // Thư viện quan trọng để đọc Device Tree

static int pcd_platform_driver_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    u32 size = 0;
    u32 perm = 0;
    const char *serial;
    int ret;

    pr_info("New device detected via Device Tree!\n");

    /* 1. Đọc số nguyên (u32) từ thuộc tính "org,size" trong .dts */
    ret = device_property_read_u32(dev, "org,size", &size);
    if (ret) {
        dev_err(dev, "Missing org,size property\n");
        return ret;
    }

    /* 2. Đọc permission */
    ret = device_property_read_u32(dev, "org,perm", &perm);
    if (ret) {
         perm = 0; // Default value
    }

    /* 3. Đọc chuỗi string */
    ret = device_property_read_string(dev, "org,serial", &serial);
    if (ret) {
        dev_warn(dev, "Missing serial number\n");
        serial = "UNKNOWN";
    }

    pr_info("Probe: Size=%d, Perm=%d, Serial=%s\n", size, perm, serial);

    /* Tiếp tục logic khởi tạo device như bình thường... */
    // ...
    return 0;
}