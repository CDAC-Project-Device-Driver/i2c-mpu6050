#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define I2C_BUS_AVAILABLE 2
#define SLAVE_DEVICE_NAME "MPU6050"
#define acc_SLAVE_ADDR 0x68

static struct i2c_adapter *desd_i2c_adapter = NULL;
static struct i2c_client *desd_i2c_client_acc = NULL;

// char driver variables
static dev_t devno;
static struct class *pclass;
static struct cdev pchar_cdev;

// File operations
static int pchar_open(struct inode *pinode, struct file *pfile);
static int pchar_close(struct inode *pinode, struct file *pfile);
static ssize_t pchar_read(struct file *pfile, char __user *ubuf, size_t bufsize, loff_t *poffset);
static ssize_t pchar_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations pchar_ops = {
    .owner = THIS_MODULE,
    .open = pchar_open,
    .release = pchar_close,
    .read = pchar_read,
    .write = pchar_write,
};

// File operations
static int pchar_open(struct inode *pinode, struct file *pfile) {
    pr_info("pchar_open() called.\n");
    return 0;
}

static int pchar_close(struct inode *pinode, struct file *pfile) {
    pr_info("pchar_close() called.\n");
    return 0;
}

static ssize_t pchar_read(struct file *pfile, char __user *ubuf, size_t bufsize, loff_t *poffset) {
    pr_info("pchar_read() called.\n");
    int8_t reg;
    int16_t accel_x,accel_y,accel_z,temp_raw,gyro_x,gyro_y,gyro_z;
    int temp_mc,temp_c;
    reg = 0x3B;
    i2c_master_send(desd_i2c_client_acc, &reg, 1);  // Just send the register address

    uint8_t data[14];
    i2c_master_recv(desd_i2c_client_acc, data, 14);  // Now read 14 bytes
    accel_x = (data[0] << 8) | data[1];
    accel_y = (data[2] << 8) | data[3];
    accel_z = (data[4] << 8) | data[5];
    temp_raw = (data[6] << 8) | data[7];
    gyro_x = (data[8] << 8) | data[9];
    gyro_y = (data[10] << 8) | data[11];
    gyro_z = (data[12] << 8) | data[13];

    pr_info("accel_x=%d, accel_y=%d, accel_z=%d\n",accel_x/16384,accel_y/16384,accel_z/16384);
    pr_info("gyro_x=%d, gyro_y=%d, gyro_z=%d\n",gyro_x/131,gyro_y/131,gyro_z/131);
    temp_mc=((int32_t)temp_raw*1000)/340+36530;
    temp_c=temp_mc/1000;
    pr_info("temp_raw= %d temp_mc=%d temp_c=%d\n",temp_raw,temp_mc,temp_c);
    pr_info("Temperature in degrees C = %d.%03d\n",temp_c,temp_mc%1000);

    copy_to_user(ubuf,data,14);
                                                                
    return 0;
}

static ssize_t pchar_write(struct file *pfile, const char __user *ubuf, size_t bufsize, loff_t *poffset) {
    pr_info("pchar_write() called.\n");
    return 0;
}

// I2C driver probe/remove
static int desd_acc_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    int ret;
    pr_info("acc I2C device probed\n");
    desd_i2c_client_acc=client;
    uint8_t reg = 0x75;              // WHO_AM_I register
    uint8_t who_am_i = 0;

    ret = i2c_master_send(client, &reg, 1);
    if (ret < 0) {
        pr_err("Failed to send WHO_AM_I register address\n");
        return ret;
    }
    ret = i2c_master_recv(client, &who_am_i, 1);
    if (ret < 0) {
        pr_err("Failed to read WHO_AM_I register\n");
        return ret;
    }
    if (who_am_i != 0x68 && who_am_i != 0x69) {
        pr_err("MPU6050 not detected! WHO_AM_I = 0x%02X\n", who_am_i);
        return -ENODEV;
    }
    dev_info(&client->dev, "MPU6050 detected. WHO_AM_I = 0x%02X\n", who_am_i);

    uint8_t buf[2] = { 0x6B, 0x00 };  // Register + data
    ret=i2c_master_send(client, buf, 2);
    if (ret < 0) {
        pr_err("Failed to send register address + data\n");
        return ret;
    }

    buf[0] = 0x1A; // CONFIG
    buf[1] = 0x01; // DLPF_CFG = 1
    ret=i2c_master_send(client, buf, 2);
    if (ret < 0) {
        pr_err("Failed to send CONFIG register address + data\n");
        return ret;
    }

    buf[0] = 0x19; // SMPRT_DIV
    buf[1] = 0x09; // SMPRT_DIV = 9
    ret=i2c_master_send(client, buf, 2);
    if (ret < 0) {
        pr_err("Failed to send SMPRT_DIV register address + data\n");
        return ret;
    }

    buf[0] = 0x6A; // USER_CTRL
    buf[1] = 0x04; // FIFO_RESET = 1
    ret=i2c_master_send(client, buf, 2);
    if (ret < 0) {
        pr_err("Failed to send USER_CTRL register address + data(FIFO_RESET)\n");
        return ret;
    }

    usleep_range(1000,2000);

    buf[0] = 0x6A; // USER_CTRL
    buf[1] = 0x40; // FIFO_EN = 1
    ret=i2c_master_send(client, buf, 2);
    if (ret < 0) {
        pr_err("Failed to send USER_CTRL register address + data(FIFO_EN)\n");
        return ret;
    }

    buf[0] = 0x23; // FIFO_EN
    buf[1] = 0x78; // GYRO_X/Y/Z + ACCEL 
    ret=i2c_master_send(client, buf, 2);
    if (ret < 0) {
        pr_err("Failed to send FIFO_EN register address + data\n");
        return ret;
    }




    if ((ret = alloc_chrdev_region(&devno, 0, 1, "pchar")) < 0)
        return ret;

    pclass = class_create(THIS_MODULE, "pchar_class");
    if (IS_ERR(pclass)) {
        unregister_chrdev_region(devno, 1);
        return PTR_ERR(pclass);
    }

    if (IS_ERR(device_create(pclass, NULL, devno, NULL, "pchar"))) {
        class_destroy(pclass);
        unregister_chrdev_region(devno, 1);
        return -1;
    }

    cdev_init(&pchar_cdev, &pchar_ops);
    if ((ret = cdev_add(&pchar_cdev, devno, 1)) < 0) {
        device_destroy(pclass, devno);
        class_destroy(pclass);
        unregister_chrdev_region(devno, 1);
        return ret;
    }

    return 0;
}

static int desd_acc_remove(struct i2c_client *client) {
    cdev_del(&pchar_cdev);
    device_destroy(pclass, devno);
    class_destroy(pclass);
    unregister_chrdev_region(devno, 1);
    pr_info("acc and char driver removed\n");
    return 0;
}

static struct of_device_id mpu6050_of_match[] = {
    { .compatible = "mpu6050" },
    { }
};
MODULE_DEVICE_TABLE(of,mpu6050_of_match);


// I2C ID and driver struct
static struct i2c_device_id desd_acc_id[] = {
    { SLAVE_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, desd_acc_id);

static struct i2c_driver desd_acc_driver = {
    .driver = {
        .name = SLAVE_DEVICE_NAME,
        .owner = THIS_MODULE,
    },
    .probe = desd_acc_probe,
    .remove = desd_acc_remove,
    .id_table = desd_acc_id,
};

// I2C board info
static struct i2c_board_info acc_i2c_board_info = {
    I2C_BOARD_INFO(SLAVE_DEVICE_NAME, acc_SLAVE_ADDR)
};

// Module init/exit
static int __init desd_driver_init(void) {
    int ret;
    pr_info("acc driver init\n");

    desd_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
    if (!desd_i2c_adapter) {
        pr_err("Failed to get I2C adapter\n");
        return -ENODEV;
    }

    desd_i2c_client_acc = i2c_new_client_device(desd_i2c_adapter, &acc_i2c_board_info);
    i2c_put_adapter(desd_i2c_adapter);

    if (IS_ERR(desd_i2c_client_acc)) {
        pr_err("Failed to register I2C client\n");
        return PTR_ERR(desd_i2c_client_acc);
    }

    ret = i2c_add_driver(&desd_acc_driver);
    if (ret < 0) {
        i2c_unregister_device(desd_i2c_client_acc);
        pr_err("Failed to add I2C driver\n");
    }

    return ret;
}

static void __exit desd_driver_exit(void) {
    pr_info("acc driver exit\n");
    i2c_del_driver(&desd_acc_driver);
    if (desd_i2c_client_acc)
        i2c_unregister_device(desd_i2c_client_acc);
}

module_init(desd_driver_init);
module_exit(desd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tejas Kale <tejas.kale137@gmail.com>");
MODULE_DESCRIPTION("Simple I2C acc driver");
