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
#include"mpu6050_ioctl.h"

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
        pr_err("FAiled tos write to PWR_MGMT_1\n");
        return ret;
    }

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
    int32_t ret;
    uint8_t reg = 0x3B;
    uint8_t data[14];
    pr_info("mpu6050: read called\n");
    if(count==0)
        return 0;
    ret=i2c_master_send(mpu6050_client,&reg,1);
    if(ret<0){
        return ret;
    }

    ret=i2c_master_recv(mpu6050_client,data,14);
    if(ret<0){
        return ret;
    }

    ret=copy_to_user(ubuf,data,count);
    return count;
}

static ssize_t mpu6050_write(struct file *file, const char __user *ubuf, size_t count, loff_t *offset){
    pr_info("mpu6050: write called\n");
    return count;
}

long mpu6050_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    mpu6050_reg_op op;
    uint8_t buf[2];
    int ret;
    switch(cmd){
        case MPU6050_WRITE_REG:
            if(copy_from_user(&op,(void *)arg,sizeof(op)))
                return -EFAULT;
            buf[0]=op.reg;
            buf[1]=op.value;
            ret=i2c_master_send(mpu6050_client,buf,2);
            if(ret<0){
                pr_err("Failed to write to MPU6050 register\n");
                return ret;
            }
        break;
        case MPU6050_READ_REG:
            if(copy_from_user(&op,(void *)arg,sizeof(op)))
                return -EFAULT;
            buf[0]=op.reg;
            ret=i2c_master_send(mpu6050_client,buf,1);
            if(ret<0){
                pr_err("Failed to write to MPU6050 register\n");
                return ret;
            }

            ret=i2c_master_recv(mpu6050_client,&op.value,1);
            if(ret<0){
                pr_err("Failed to read register value from MPU6050\n");
                return ret;
            }

            if(copy_to_user((void*)arg,&op,sizeof(op)))
                return -EFAULT;
        break;
        default:
            pr_err("%s: invalid cmd in mpu6050_ioctl()\n", THIS_MODULE->name);
            return -EINVAL;
    }
    return 0;
}

static const struct file_operations mpu6050_fops = {
    .owner   = THIS_MODULE,
    .open    = mpu6050_open,
    .release = mpu6050_release,
    .read    = mpu6050_read,
    .write   = mpu6050_write,
    .unlocked_ioctl=mpu6050_ioctl
};

/*      I2C Driver Functions        */
static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id){
    int ret;
    uint8_t reg = 0x75;              // WHO_AM_I register
    uint8_t who_am_i = 0;
    pr_info("acc I@C device probed\n");
    mpu6050_client = client;

    ret=i2c_master_send(client,&reg,1);
    if(ret<0){
        pr_err("Failed to send WHA+AM_I register address\n");
        return ret;
    }
    ret=i2c_master_recv(client,&who_am_i,1);
    if(ret<0){
        pr_err("Failed to read WHA+AM_I register\n");
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
    pr_info("%s: device number = %d/%d.\n",THIS_MODULE->name,MAJOR(mpu6050_devno),MINOR(mpu6050_devno));

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
// static const struct of_device_id mpu6050_of_match[] = {
//     { .compatible = "invensense,mpu6050" },
//     { }
// };
// MODULE_DEVICE_TABLE(of, mpu6050_of_match);

static const struct i2c_device_id mpu6050_id[] = {
    { SLAVE_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c,mpu6050_id);

static struct i2c_driver mpu6050_i2c_driver={
    .driver = {
        .name = SLAVE_DEVICE_NAME,
        .owner = THIS_MODULE,
        // .of_match_table = mpu6050_of_match,
    },
    .probe = mpu6050_probe,
    .remove = mpu6050_remove,
    .id_table = mpu6050_id,
};

static struct i2c_board_info mpu6050_board_info = {
    I2C_BOARD_INFO(SLAVE_DEVICE_NAME, SLAVE_ADDR)
};

/*      Module Init & Exit      */
static int __init mpu6050_init(void){
    int ret;

    pr_info("%s: mpu6050_init() called.\n", THIS_MODULE->name);

    mpu6050_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
    if(!mpu6050_i2c_adapter){
        pr_err("%s: Failed to get I2C adapter %d\n",THIS_MODULE->name,I2C_BUS_AVAILABLE);
        return -ENODEV;
    }

    mpu6050_client = i2c_new_client_device(mpu6050_i2c_adapter,&mpu6050_board_info);
    i2c_put_adapter(mpu6050_i2c_adapter);

    if(IS_ERR(mpu6050_client)){
        pr_err("%s: Failed to create I2C client\n",THIS_MODULE->name);
        return PTR_ERR(mpu6050_client);
    }

    ret = i2c_add_driver(&mpu6050_i2c_driver);
    if(ret < 0){
        pr_err("%s: Failed to add I2C driver\n",THIS_MODULE->name);
        i2c_unregister_device(mpu6050_client);
        return ret;
    }

    pr_info("%s: I2C driver loaded successfully\n",THIS_MODULE->name);
    return 0;
}

//module de-initialization function
static void __exit mpu6050_exit(void){
    pr_info("%s: mpu6050_exit() called.\n", THIS_MODULE->name);

    i2c_del_driver(&mpu6050_i2c_driver);

    if(mpu6050_client)
        i2c_unregister_device(mpu6050_client);
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tejas Kale <tejas.kale137@gmail.com>");
MODULE_DESCRIPTION("I2C driver for MPU6050 accelerometer and gyroscope sensor");