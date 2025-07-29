#include<linux/device.h>		//device model support for device_create and class_create
#include<linux/module.h>		//required for __init and __exit()
#include<linux/cdev.h>			//reqd for character device registration
#include<linux/fs.h>			//file system interface file_ops
#include<linux/kernel.h>		
#include"i2c_mpu6050_ioctl.h"	//use to add ioctl macro and command
#include<linux/ioctl.h>			//use to implement ioctl function
#include<linux/proc_fs.h>		//use to create proc file sys under /proc/
#include<linux/seq_file.h>		
#include<linux/uaccess.h>		//user to kernel  copy_to and copy_from
#include<linux/i2c.h>			//use to implement i2c function

#define PROC_NAME  					"mpu6050_info"
#define I2C_BUS_AVAILABLE 			2
#define SLAVE_DEVICE_NAME 			"MPU_6050"
#define SLAVE_ADDRESS 				0x68
#define ACCEL_CLASS_NAME 			"accelerometer_class"
#define ACCEL_DEVICE_NAME 			"Accelerometre_MPU6050"
#define WHO_AM_I_REG				0x75
#define PWR_MGMT_1_REG				0x6B
#define ACCEL_X_OUT_H_REG			0x3B
#define GYRO_X_OUT_H_REG			0x43
#define ACCEL_CONFIG_REG			0x1C
#define GYRO_CONFIG_REG				0x1B
#define ACC_FS_SENSITIVITY_0 		16384
#define ACC_FS_SENSITIVITY_1        8192
#define ACC_FS_SENSITIVITY_2		4096
#define ACC_FS_SENSITIVITY_3		2048
#define GYRO_FS_SENSITIVITY_0 		1310
#define GYRO_FS_SENSITIVITY_1		655
#define GYRO_FS_SENSITIVITY_2		328
#define GYRO_FS_SENSITIVITY_3		164

static struct proc_dir_entry *mpu6050_proc_entry=NULL;
static struct i2c_adapter *i2c_adapter_accel=NULL:
static struct i2c_client *i2c_client_accel=NULL;
static struct class *pclass=NULL;
static dev_t devno;
static struct cdev accel_cdev;
static int accel_sensitivity=ACC_FS_SENSITIVITY_0;
static int gyro_sensitivity=GYRO_FS_SENSITIVITY_0;
static int mpu_proc_display(struct seq_file *m, void *v);
static int accel_open(struct inode *pinode, struct file *pfile);
static int accel_close(struct inode *pinode, struct file *pfile);
static ssize_t accel_read(struct file *pfile,char __user *ubuf, size_t bufsize, loff_t *poffset);
static long accel_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg);
static int accel_probe(struct i2c_client *client,const struct i2c_device_id *id);
static int accel_remove(struct i2c_client *client);

static struct file_operations accel_ops={
	.owner=THIS_MODULE,
	.open=accel_open,
	.release=accel_close,
	.read=accel_read,
	.unlocked_ioctl=accel_ioctl,
};


static int mpu_proc_open(struct inode *pinode, struct file *pfile)
{
	pr_info("mpu6050_info proc file is opened in procfs");
	return 0;
}

static const struct proc_ops mpu_proc_ops={
	.proc_open=mpu_proc_open,
	.proc_read=seq_read,
};

static const struct of_device_id mpu6050_match_ids[]={
	{.compatible = "invensense,mpu6050"},{/*sentinel*/}
};

MODULE_DEVICE_TABLE(of,mpu6050_match_ids);

static const struct i2c_device_id accelerometre_id[]={
	{SLAVE_DEVICE_NAME,0}
	{ }
};

MODULE_DEVICE_TABLE(i2c,accelerometre_id);

static struct i2c_driver accel_driver={
	.driver={
			.name=SLAVE_DEVICE_NAME,
			.owner=THIS_MODULE,
	},
	.probe=accel_probe,
	.remove=accel_remove,
	.id_table=accel_id,

};

static struct i2c_board_info accel_board_info= {
	I2C_BOARD_INFO(SLAVE_DEVICE_NAME,SLAVE_ADDRESS)
};

static int __init accel_init()
{
	int ret=-1;
	pr_info("%s Accel_init is started\n",THIS_MODULE->name);
	i2c_adapter_accel=i2c_get_adapter(I2C_BUS_AVAILABLE);
	if(i2c_adapter_accel!=NULL)
	{
		i2c_client_accel=i2c_new_client_device(i2c_adapter_accel,&accel_board_info);
		if(i2c_client_accel!=NULL)
			{
				i2c_add_driver(&accel_driver);
				pr_info("%s driver for the device added successfully",THIS_MODULE->name);
			}
		else
			pr_info("Accelerometre not detected");
		i2c_put_driver(i2c_adapter_accel);
	}
	else	
		pr_info("%s I2C Bus Adapter is not availabe",THIS_MODULE->name);
	pr_info("%s Accel_init() is done successfully\n",THIS_MODULE->name);
	return 0;
}

static void __exit accel_exit()
{
	pr_info("%s Accel_exit() is called\n",THIS_MODULE->name);
	if(i2c_client_accel!=NULL)
	{
		i2c_unregister_device(i2c_client_accel);
		i2c_del_driver(&accel_driver);
	}
	pr_info("Driver removed successfully");
	pr_info("%s Accel_exit() is completed\n",THIS_MODULE->name);
}

static int accel_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	static struct device *pdevice;
	int ret;
	uint8_t who_am_i;
	uint8_t accel_config;
	uint8_t gyro_config;
	i2c_client_accel=client;

	pr_info("%s accel_probe() is started\n",THIS_MODULE->name);
	who_am_i=i2c_smbus_read_byte_data(client,WHO_AM_I_REG);	//to check device got connected
		if(who_am_i!=0x68 && who_am_i !=0x69)
		{
			pr_err("MPU6050 not detected WHO_AM_I = 0x%02X\n",who_am_i); //as the device is not connected
			return -ENODEV;
		}
	pr_info("%s MPU6050 detected. WHO_AM_I = 0x%02X\n",THIS_MODULE->name,who_am_i);

	ret= i2c_smbus_write_byte_data(client,PWR_MGMT_1_REG,0x00);
	if(ret<0)
		{
			pr_err("Failed to Power ON MPU6050");
			return -EIO;
		}
	accel_config = i2c_smbus_read_byte_data(client,ACCEL_CONFIG_REG);
	if(accel_config<0)
		accel_config=0;
	accel_sensitivity = ACC_FS_SENSITIVITY_0 >>((accel_config>>3) & 0x03);	//set accel_config range as it is starting from bit 3 thats why shifted by 3.
	switch((accel_config >> 3) & 0x03){
	case 0: accel_sensitivity = ACC_FS_SENSITIVITY_0; break;
	case 1: accel_sensitivity = ACC_FS_SENSITIVITY_1; break;
	case 2: accel_sensitivity = ACC_FS_SENSITIVITY_2; break;
	case 3: accel_sensitivity = ACC_FS_SENSITIVITY_3; break;
	}
	
	gyro_config = i2c_smbus_read_byte_data(client,GYRO_CONFIG_REG);
	if(gyro_config<0)
		gyro_config=0;		//default value to be set to 0
	gyro_sensitivity = GYRO_FS_SENSITIVITY_0 >>((gyro_config>>3) & 0x03);	//set gyro_config range as it is starting from bit 3 thats why shifted by 3.
	switch((gyro_config >> 3) & 0x03){
	case 0: gyro_sensitivity = GYRO_FS_SENSITIVITY_0; break;
	case 1: gyro_sensitivity = GYRO_FS_SENSITIVITY_1; break;
	case 2: gyro_sensitivity = GYRO_FS_SENSITIVITY_2; break;
	case 3: gyro_sensitivity = GYRO_FS_SENSITIVITY_3; break;
	}

	pr_info("Accelerometer Sensitivity = %d , Gyrometer Sensitivity = %d\n",accel_sensitivity,gyro_sensitivity);

	ret = alloc_chrdev_region(&devno,0,1,ACCEL_DEVICE_NAME);
	if(ret<0)
		{
			pr_err("%s alloc_chrdev_region() is failed\n",THIS_MODULE->name);
			goto alloc_chrdev_region_failed;
		}
	pr_info("%s Device Number is MAJOR= %d and MINOR = %d having device name = %s\n",THIS_MODULE->name,MAJOR(devno),MINOR(devno),ACCEL_DEVICE_NAME);
	
	pclass = class_create(THIS_MODULE,ACCEL_CLASS_NAME);
	if(IS_ERR(pclass))
	{
		pr_err("%s class_create() is failed\n",THIS_MODULE->name);
		goto class_create_failed;
	}
	pr_info("%s class is created successfully of name %s\n",THIS_MODULE->name,ACCEL_CLASS_NAME);

	pdevice = device_create(pclass, NULL,devno,NULL,ACCEL_DEVICE_NAME);
	// device_create(struct class *class, struct device *parent=NULL,dev_t devt,void *drvdata, const char *fmt=string for device name);	
	//drvdata=data to be added to the device for callback so i kept it as NULL as nothing to be added.
	if(IS_ERR(pdevice))
	{
		pr_err("%s device_create is failed\n",THIS_MODULE->name);
		goto device_create_failed;
	}
	pr_info("%s device is created successfully of name %s",THIS_MODULE->name,ACCEL_DEVICE_NAME);
	
	cdev_init(&accel_cdev,&accel_ops);
	ret = cdev_add(&accel_cdev,devno,1);
	if(ret<0)
	{
		pr_err("%s cdev_add() is failed\n",THIS_MODULE->name);
		goto cdev_add_failed;
	}
	pr_info("%s cdev_add is done successfully\n",THIS_MODULE->name);
	
	mpu6050_proc_entry = proc_create(PROC_NAME,0444,NULL, &mpu_proc_ops);
	if(!mpu6050_proc_entry)
		{
			pr_err("Failed to create /proc/%s\n",PROC_NAME);
			goto proc_create_failed;
		}
	pr_info("Accel_probe() is completed\n");
	
	return 0;
	proc_create_failed:
		cdev_del(&accel_cdev);
	cdev_add_failed:
		device_destroy(pclass,devno);
	device_create_failed:
		class_destroy(pclass);
	class_create_failed:
	unregister_chrdev_region(devno,1);
	alloc_chrdev_region_failed:
	return ret;
}

static int accel_remove(struct i2c_client *client)
{
	if(mpu6050_proc_entry)
	{
		proc_remove(mpu6050_proc_entry);
		pr_info("/proc/%s removed",PROC_NAME);
	}
	pr_info("%s accel_remove is called\n",THIS_MODULE->name);
	cdev_del(&accel_cdev);
	pr_info("%s cdev_del() is done successfully\n",THIS_MODULE->name);
	device_destroy(pclass,devno);
	pr_info("%s device_destroy() is done successfully\n",THIS_MODULE->name);
	class_destroy(pclass);
	pr_info("%s class_destroy() is done successfully\n",THIS_MODULE->name);
	unregister_chrdev_region(devno,1);
	pr_info("%s unregister_chrdev_region() is done successfully\n",THIS_MODULE->name);
	pr_info("accel_remove is completed\n");
	return 0;
}

static int accel_open(struct inode *pinode, struct file *pfile)
{
	pr_info("Accel_open() is done successfully\n");	
	return 0;
}

static int accel_close(struct inode *pinode, struct file *pfile)
{
	pr_info("accel_close is done successfully\n");
	return 0;
}
static ssize_t accel_read(struct file *pfile,char __user *ubuf, size_t bufsize, loff_t *poffset)
{
	uint8_t i=0;
	uint8_t data[14]; 
	int16_t temp_raw,accel_x_raw,accel_y_raw,accel_z_raw, gyro_x_raw, gyro_y_raw, gyro_z_raw;
	int16_t accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z;
	int temp_mc,temp_c;
	pr_info("%s accel_read() is called");
	
	for(i=0;i<14;i++)
	{
		int ret = i2c_smbus_read_byte_data(i2c_client_accel,ACCEL_X_OUT_H_REG+i);
		if(ret<0)
			return -EIO;		//return error if it fails to read the data
		data[i] = ret;		//it assures that it reads every byte as the loop goes.
	}
	//MPU6050 gives data in  big endianness 16 bit signed integer.
	//each axis and temp values are splitted into two bytes so as per below we have to convert the data;
	accel_x_raw = (data[0]<<8)|data[1];
	accel_y_raw = (data[2]<<8)|data[3];
	accel_z_raw = (data[4]<<8)|data[5];
	temp_raw	= (data[6]<<8)|data[7];
	gyro_x_raw	= (data[8]<<8)|data[9];
	gyro_y_raw 	= (data[10]<<8)|data[11];
	gyro_z_raw	= (data[12]<<8)|data[13];

	accel_x = (accel_x_raw*1000)/accel_sensitivity;
	accel_y = (accel_y_raw*1000)/accel_sensitivity;
	accel_z	= (accel_z_raw*1000)/accel_sensitivity;
	gyro_x	= (gyro_x_raw*1000)/accel_sensitivity;
	gyro_y	= (gyro_y_raw*1000)/accel_sensitivity;
	gyro_z	= (gyro_z_raw*1000)/accel_sensitivity;

	temp_mc = ((int32_t)temp_raw*1000)/340 +36530;
	temp_c 	= temp_mc/1000;	

    pr_info("Accelerometer: x=%d g y=%d g z=%d g\n", accel_x, accel_y, accel_z);
    pr_info("Gyrometer 	  : x=%d °/s y=%.d °/s z=%d °/s\n", gyro_x, gyro_y, gyro_z);
    pr_info("Temparature  : %d.%03d deg C\n", temp_c, temp_mc % 1000);

	if(copy_to_user(ubuf,data,14))
		return -EFAULT;
	return 14;	//number of byte transferred is returned.
}


static long accel_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
	int ret;
	int16_t value;

	switch(cmd)
		{
			case MPU6050_READ_ACCEL_X:
				ret=i2c_smbus_read_word_data(i2c_client_accel,0x3B);
				break;
			case MPU6050_READ_ACCEL_Y:
				ret=i2c_smbus_read_word_data(i2c_client_accel,0x3D);
				break;
			case MPU6050_READ_ACCEL_Z:
				ret =i2c_smbus_read_word_data(i2c_client_accel,0x3F);
				break;
			case MPU6050_READ_TEMP:
				ret = i2c_smbus_read_word_data(i2c_client_accel,0x41);
				break;
			case MPU6050_READ_GYRO_X:
				ret = i2c_smbus_read_word_data(i2c_client_accel,0x43);
				break;
			case MPU6050_READ_GYRO_Y:
				ret = i2c_smbus_read_word_data(i2c_client_accel,0x45);
				break;
			case MPU6050_READ_GYRO_Z:
				ret = i2c_smbus_read_word_data(i2c_client_accel,0x47);
				break;
			default:
				return -EINVAL;
		}	
		if(ret<0)
			return ret;
		value =((ret & 0xFF)<<8) | ((ret>>8) &0xFF);

		if(cmd== MPU6050_READ_TEMP)
			{
				value= (value * 100)/340 + 3653;
			}
		if(copy_to_user((int16_t __user*)arg,&value,sizeof(value)))
			return -EFAULT;
					
		return 0;			
}


static int mpu_proc_display(struct seq_file *m, void *v)
{
	int16_t ax, ay,az,gx,gy,gz;
	uint8_t data[14];
	int i,ret;
	
	for(i=0;i<14;i++)
	{
		ret = i2c_smbus_read_byte_data(i2c_client_accel,ACCEL_X_OUT_H_REG+i);
		if(ret<0)
		{
			return ret;
		}
		data[i]=ret;
	}

	ax = (data[0] << 8) | data[1];
    ay = (data[2] << 8) | data[3];
    az = (data[4] << 8) | data[5];
    t  = (data[6]<<8)   | data[7];
    gx = (data[8] << 8) | data[9];
    gy = (data[10] << 8)| data[11];
    gz = (data[12] << 8)| data[13];


    seq_printf(m, "Accelerometer (mg): X=%d Y=%d Z=%d\n",
               (ax * 1000) / accel_sensitivity,
               (ay * 1000) / accel_sensitivity,
               (az * 1000) / accel_sensitivity);

    seq_printf(m, "Gyroscope (mdps): X=%d Y=%d Z=%d\n",(gx * 1000) / gyro_sensitivity,(gy * 1000) / gyro_sensitivity,(gz * 1000) / gyro_sensitivity);

    seq_printf(m,"Temp: %d.%03d deg C\n",t*1000);
	return 0;
}

module_init(accel_init);
module_exit(accel_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chetan S. Sunaskar <chetansunaskar@gmail.com>");
MODULE_DESCRIPTION("Embedded Linux Device driver for MPU6050 using smbus api to read 6 axis accelerometer reading, gyroscope reading and temp also created procfs by using kernel api");
