#include<linux/device.h>
#include<linux/module.h>
#include<linux/slab.h>
#include<linux/cdev.h>
#include<linux/i2c.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/delay.h>

#define I2C_BUS_AVAILABLE 2
#define SLAVE_DEVICE_NAME "MPU_6050"
#define SLAVE_ADDRESS   0x68
#define ACCEL_CLASS_NAME  "accel_class"
#define ACCEL_DEVICE_NAME "accelerometre"

static struct i2c_adapter *i2c_adapter_accel=NULL;
static struct i2c_client *i2c_client_accel=NULL;
static struct class *pclass=NULL;
static dev_t devno;
static struct cdev accel_cdev;

static int accel_open(struct inode *pinode,struct file *pfile);
static int accel_close(struct inode *pinode,struct file *pfile);
static ssize_t accel_read(struct file *pfile,char __user *ubuf,size_t bufsize, loff_t *poffset);

static struct file_operations accel_ops={
    .owner=THIS_MODULE,
    .open=accel_open,
    .release=accel_close, 
    .read= accel_read,
};

static const struct i2c_device_id accel_id[]={
    { SLAVE_DEVICE_NAME ,0},
    { }
};
MODULE_DEVICE_TABLE(i2c,accel_id);

static int accel_probe(struct i2c_client *client,const struct i2c_device_id *id);
static int accel_remove(struct i2c_client *client);

static struct i2c_driver accel_driver = {
        .driver = {
            .name   = SLAVE_DEVICE_NAME,
            .owner  = THIS_MODULE,
        },
        .probe          = accel_probe,
        .remove         = accel_remove,
        .id_table       = accel_id,
};

static struct i2c_board_info accel_board_info ={
    I2C_BOARD_INFO(SLAVE_DEVICE_NAME, SLAVE_ADDRESS)
};

static int __init accel_init(void)
{
    pr_info("%s accel_init() is started\n",THIS_MODULE->name);
    int ret=-1;
    i2c_adapter_accel=i2c_get_adapter(I2C_BUS_AVAILABLE);  
    if(i2c_adapter_accel!=NULL)
    {
        i2c_client_accel=i2c_new_client_device(i2c_adapter_accel,&accel_board_info);
        if(i2c_client_accel!=NULL)
        {
            i2c_add_driver(&accel_driver);
            pr_info("%s Driver Added Successfully\n",THIS_MODULE->name);
            ret=0;
        }
        else{
            pr_info("accel not detected !!!\n");
        }
        i2c_put_adapter(i2c_adapter_accel);
    }
    else
        pr_info("%s I2C bus adapter not available\n",THIS_MODULE->name);

    pr_info("%s Accel_init() is done successfully\n",THIS_MODULE->name);
    return 0;
}

static void __exit accel_exit(void)
{   
    pr_info("%s accel_exit() is called\n",THIS_MODULE->name);
        if(i2c_client_accel!=NULL)
        {
            i2c_unregister_device(i2c_client_accel);
            i2c_del_driver(&accel_driver);
        }
    pr_info("%s Driver Removed Successfully\n",THIS_MODULE->name);
    pr_info("%s accel_exit() is completed\n",THIS_MODULE->name);
}


static int accel_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    static struct device *pdevice;
    int ret;
    
    //accel_init() need to be done\n
    uint8_t reg=0x75;
    uint8_t find=0;  //who_am_i register in accelerometre
    pr_info("%s accel_probe() is called\n",THIS_MODULE->name);
    ret=i2c_master_send(client,&reg,1);
        if(ret<0)
        {
             pr_err("%s failed to send who_am_i register address\n",THIS_MODULE->name);
                return -EIO;
        }
    ret=i2c_master_recv(client,&find,1);
        if(ret<0)
        {
            pr_err("Failed to read WHO_AM_I register\n");
            return -EIO;
        }

        if(find!=0x68 && find!=69)
        {
            pr_err("MPU6050 not detected! WHO_AM_I = 0x%02X\n", find);
            return -ENODEV;
        }
    dev_info(&client->dev, "MPU6050 detected. WHO_AM_I = 0x%02x\n", find);
    uint8_t buf[2]={0x6B,0x00};         //6B --For PWR_MGMT
    ret=i2c_master_send(i2c_client_accel,buf,2);
    
    ret=alloc_chrdev_region(&devno,0,1,ACCEL_DEVICE_NAME);
        if(ret<0)
        {   
            pr_err("%s alloc_chrdev_region() is failed\n",THIS_MODULE->name);
            goto alloc_chrdev_region_failed;
        }
    pr_info("%s Device Number is MAJOR=%d /MINOR=%d,device name:- %s\n",THIS_MODULE->name,MAJOR(devno),MINOR(devno),ACCEL_DEVICE_NAME);
    pclass=class_create(THIS_MODULE, ACCEL_CLASS_NAME);
        if(IS_ERR(pclass))
        {
            pr_err("%s class_create() is failed\n",THIS_MODULE->name);
            goto class_create_failed;
        }
    pr_info("%s class is created successfully of name %s",THIS_MODULE->name,ACCEL_CLASS_NAME);
    pdevice=device_create(pclass,NULL,devno,NULL,ACCEL_DEVICE_NAME);
        if(IS_ERR(pdevice))
        {
            pr_err("%s device_create() is failed\n",THIS_MODULE->name);
            goto device_create_failed;
        }
    pr_info("%s device is created successfully of name %s",THIS_MODULE->name,ACCEL_DEVICE_NAME);
    
    cdev_init(&accel_cdev,&accel_ops);
    ret=cdev_add(&accel_cdev,devno,1);
        if(ret<0)
        {
            pr_err("%s cdev_add() failed\n",THIS_MODULE->name);
            goto cdev_add_failed;
        }
    pr_info("%s cdev_add is successfull\n",THIS_MODULE->name);
    pr_info("%s accel_probe() is completed\n",THIS_MODULE->name);
    
    return 0;
    cdev_add_failed:
        device_destroy(pclass,devno);
    device_create_failed:
        class_destroy(pclass);
    class_create_failed:
        unregister_chrdev_region(devno,1);
    alloc_chrdev_region_failed:
        return ret;
}
static int accel_remove(struct i2c_client *client){
    pr_info("%s accel_probe() is called\n",THIS_MODULE->name);
    cdev_del(&accel_cdev);
    pr_info("%s cdev_del() is done successfully\n",THIS_MODULE->name);
    device_destroy(pclass,devno);
    pr_info("%s device_destroy() is done successfully\n",THIS_MODULE->name);
    class_destroy(pclass);
    pr_info("%s class_destroy() is done successfully\n",THIS_MODULE->name);
    unregister_chrdev_region(devno,1);
    pr_info("%s unregister_chrdev_region() is done successfully\n",THIS_MODULE->name);
    pr_info("%s accel_remove() is completed\n",THIS_MODULE->name);
    return 0;
}


static int accel_open(struct inode *pinode,struct file *pfile){

    pr_info("%s accel_open() is done successfully\n",THIS_MODULE->name);
    return 0;
}

static int accel_close(struct inode *pinode,struct file *pfile)
{
    pr_info("^%s accel_close() is done successfully\n",THIS_MODULE->name);
    return 0;
}
static ssize_t accel_read(struct file *pfile,char __user *ubuf,size_t bufsize, loff_t *poffset)
{   
    pr_info("%s accel_read() called\n",THIS_MODULE->name);
    int8_t reg;
    int8_t reg1=0x43;
    int accel_x,accel_y,accel_z;
    reg=0x3B;
    char kbuf[128];
    int gyro_x,gyro_y,gyro_z;
    i2c_master_send(i2c_client_accel,&reg,1); //address transfer from i2c frame.
    uint8_t data[6];
    uint8_t val[6];
    int len;
    i2c_master_recv(i2c_client_accel,data,6);
    accel_x=(data[0]<<8) | data[1];
    accel_y=(data[2]<<8) | data[3];
    accel_z=(data[4]<<8) | data[5];

    pr_info("%s Accel_x =%d, Accel_y =%d, Accel_z = %d\n",THIS_MODULE->name,accel_x,accel_y,accel_z);
    if (i2c_master_send(i2c_client_accel, &reg1, 2) != 2) {
        pr_err("Failed to write gyro register\n");
        return -EIO;
    }

    if (i2c_master_recv(i2c_client_accel, val, 6) != 6) {
        pr_err("Failed to write gyro register\n");
        return -EIO;
    }
    gyro_x=(val[0]<<8)|val[1];
    gyro_y=(val[2]<<8)|val[3];
    gyro_z=(val[4]<<8)|val[5];
    pr_info("%s  gyro_x =%d, gyro_y =%d, gyro_z = %d\n",THIS_MODULE->name,gyro_x,gyro_y,gyro_z); 
    
    len=snprintf(kbuf, sizeof(kbuf),"Accel x=%d,y=%d,z=%d\nGyro x=%d,y=%d,z=%d\n",accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z);

    if(copy_to_user(ubuf,kbuf,len))
        return -EINVAL;
    return len;
}

module_init(accel_init);
module_exit(accel_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chetansunaskar@gmail.com");
MODULE_DESCRIPTION("I2C DEVICE DRIVER FOR ACCELEROMETRE");
