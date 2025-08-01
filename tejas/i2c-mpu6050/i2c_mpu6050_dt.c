#include<linux/module.h>
#include<linux/init.h>
#include<linux/i2c.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/uaccess.h>
#include<linux/slab.h>
#include<linux/delay.h>

#define I2C_BUS_AVAILABLE 2
#define SLAVE_DEVICE_NAME "mpu6050"
#define SLAVE_ADDR 0x68

/* I2C client and adapter pointers*/
static struct i2c_adapter *mpu6050_i2c_adapter;
static struct i2c_client *mpu6050_client;

/* Character device variables */
static dev_t mpu6050_devno;
static struct class *mpu6050_class;
static struct cdev mpu6050_cdev;

static int mpu6050_initialize_device(struct i2c_client *client){
    int ret;
    uint8_t buf[2]={0x6B,0x00};//Register + data
    ret=i2c_master_send(client,buf,2);
    if(ret<0){
        pr_err("Failed to write to PWR_MGMT_1\n");
        return ret;
    }

    buf[0]=0x1C;//ACCEL_CONFIG = 2 g
    buf[1]=0x00;
    ret=i2c_master_send(client,buf,2);

    buf[0]=0x1B;//GYRO_CONFIG = 250 degree/s
    buf[1]=0x00;
    ret=i2c_master_send(client,buf,2);

    buf[0]=0x1A;//CONFIG: DLPF 42Hz
    buf[1]=0x03;
    ret=i2c_master_send(client,buf,2);

    buf[0]=0x19;//SMPLRT_DIV = 7 -> 125 Hz
    buf[1]=0x07;
    ret=i2c_master_send(client,buf,2);

    pr_info("MPU6050 successfully initialized\n");
    return 0;
}
/*      File Operations     */
int mpu6050_open(struct inode *inode, struct file *file){
    pr_info("mpu6050: device opened\n");
    return 0;
}

static int mpu6050_release(struct inode *inode, struct file *file)
{
    pr_info("mpu6050: device closed\n");
    return 0;
}

static ssize_t mpu6050_read(struct file *file, char __user *ubuf, size_t count, loff_t *offset){
    pr_info("mpu6050: read called\n");
    return count;
}

static ssize_t mpu6050_write(struct file *file, char __user *ubuf, size_t count, loff_t *offset){
    pr_info("mpu6050: write called\n");
    return count;
}

static const struct file_operations mpu6050_fops = {
    .owner   = THIS_MODULE,
    .open    = mpu6050_open,
    .release = mpu6050_release,
    .read    = mpu6050_read,
    .write   = mpu6050_write,
};

/*      I2C Driver Functions        */
static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id){
    int ret;
    pr_info("mpu6050 device probed\n");
    mpu6050_client = client;
    uint8_t reg = 0x75;              // WHO_AM_I register
    uint8_t who_am_i = 0;

    ret=i2c_master_send(client,&reg,1);
    if(ret<0){
        pr_err("Failed to send WHO_AM_I register address\n");
        return ret;
    }
    ret=i2c_master_recv(client,&who_am_i,1);
    if(ret<0){
        pr_err("Failed to read WHO_AM_I register\n");
        return ret;
    }
    if(who_am_i!=0x68 && who_am_i!=0x69){
        pr_err("MPU6050 not detected!WHO_AM_I=0x%02X\n",who_am_i);
        return -ENODEV;
    }
    dev_info(&client->dev,"MPU6050 detected!WHO_AM_I=0x%02X\n",who_am_i);

    ret = mpu6050_initialize_device(client);
    if(ret<0)
        return ret;

    /* Char device registration */

    ret = alloc_chrdev_region(&mpu6050_devno, 0, 1, "mpu6050");
    if(ret<0)
        return ret;
    pr_info("%s: device number = %d/%d.\n",THIS_MODULE,MAJOR(mpu6050_devno),MINOR(mpu6050_devno));

    mpu6050_class = class_create(THIS_MODULE,"mpu6050_class");
    if(IS_ERR(mpu6050_class)){
        unregister_chrdev_region(mpu6050_devno,1);
        return PTR_ERR(mpu6050_class);
    }

    if (IS_ERR(device_create(mpu6050_class, NULL, mpu6050_devno, NULL, "mpu6050"))) {
        class_destroy(mpu6050_class);
        unregister_chrdev_region(mpu6050_devno, 1);
        return -1;
    }

    cdev_init(&mpu6050_cdev,&mpu6050_fops);
    ret = cdev_add(&mpu6050_cdev,mpu6050_devno,1);
    if(ret<0){
        device_destroy(mpu6050_class,mpu6050_devno);
        class_destroy(mpu6050_class);
        unregister_chrdev_region(mpu6050_devno,1);
    }

    return ret;
}

static int mpu6050_remove(struct i2c_client *client)
{
    cdev_del(&mpu6050_cdev);
    device_destroy(mpu6050_class, mpu6050_devno);
    class_destroy(mpu6050_class);
    unregister_chrdev_region(mpu6050_devno, 1);
    pr_info("mpu6050: driver removed\n");
    return 0;
}

static const struct of_device_id mpu6050_of_match[] = {
    { .compatible = "invensense,mpu6050" },
    { }
};
MODULE_DEVICE_TABLE(of, mpu6050_of_match);

static const struct i2c_device_id mpu6050_id[] = {
    { SLAVE_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c,mpu6050_id);

static struct i2c_driver mpu6050_i2c_driver={
    .driver = {
        .name = SLAVE_DEVICE_NAME,
        .owner = THIS_MODULE,
        .of_match_table = mpu6050_of_match,
    },
    .probe = mpu6050_probe,
    .remove = mpu6050_remove,
    .id_table = mpu6050_id,
};

/*      Module Init & Exit      */
static int __init mpu6050_init(void){
    return i2c_add_driver(&mpu6050_i2c_driver);
}

//module de-initialization function
static void __exit mpu6050_exit(void){
    return i2c_del_driver(&mpu6050_i2c_driver);
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tejas Kale <tejas.kale137@gmail.com>");
MODULE_DESCRIPTION("I2C driver for MPU6050 accelerometer and gyroscope sensor");