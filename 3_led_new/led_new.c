#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/cdev.h>

#define NEWCHRLED_CNT 1      /*设备个数*/
#define NEWCHRLED_NAME "newchrled"


/*
 *GPIO物理地址
 */
#define ZYNQ_GPIO_REG_BASH   0xE000A000
#define DATA_OFFSET          0x00000040
#define DRIM_OFFSET          0x00000204
#define OUTEN_OFFSET         0x00000208
#define INTDIS_OFFSET        0x00000214
#define APER_CLK_CTRL        0xF800012C

/*
 *映射后的GPIO地址
 */
static void __iomem *data_addr;
static void __iomem *drim_addr;
static void __iomem *outen_addr;
static void __iomem *intdis_addr;
static void __iomem *aper_clk_ctrl_addr;

/*
 *新字符设备结构
 */
struct newchrled_dev{
	dev_t devid;                   /*设备id*/
	struct cdev cdev;              /*字符设备*/
	struct class *class;           /*设备所属类*/
	struct device *device;         /*设备*/
	int major;                     /*主设备号*/
	int minor;                     /*此设备号*/
};
struct newchrled_dev newchrled;

static int newchrled_open (struct inode *inode, struct file *filp)
{
	filp->private_data=&newchrled;
	return 0;
}

static int newchrled_release (struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t newchrled_read (struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}

static ssize_t newchrled_write (struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int ret;
	int val;
	char kern_buf[1];
	ret = copy_from_user(kern_buf,buf,cnt);      /*得到应用层传来的数据*/
	if(0>ret)
	{
		printk(KERN_ERR"kernel write failed!\r\n");
		return -EFAULT;
	}
	val=readl(data_addr);
	if(0==kern_buf[0])
	{
		val&=~(1<<7);
	}
	else if(1==kern_buf[0])
	{
		val|=(1<<7);
	}
	writel(val,data_addr);
	return 0;
	
}

static inline void led_ioremap(void)
{
	data_addr=ioremap(ZYNQ_GPIO_REG_BASH+DATA_OFFSET,4);
	drim_addr=ioremap(ZYNQ_GPIO_REG_BASH+DRIM_OFFSET,4);
	outen_addr=ioremap(ZYNQ_GPIO_REG_BASH+OUTEN_OFFSET,4);
	intdis_addr=ioremap(ZYNQ_GPIO_REG_BASH+INTDIS_OFFSET,4);
	aper_clk_ctrl_addr=ioremap(APER_CLK_CTRL,4);
}

static inline void led_iounmap(void)
{
	iounmap(data_addr);
	iounmap(drim_addr);
	iounmap(outen_addr);
	iounmap(intdis_addr);
	iounmap(aper_clk_ctrl_addr);
}

struct file_operations fops={
	.owner=THIS_MODULE,
	.open=newchrled_open,
	.read=newchrled_read,
	.release=newchrled_release,
	.write=newchrled_write,
};

static int __init newchrled_init(void)
{
	int ret;
	u32 val;
	
	/*内存映射*/
	led_ioremap();
	
	/*gpio时钟使能*/
	val=readl(aper_clk_ctrl_addr);
	val|=(0x1U<<22);
	writel(val,aper_clk_ctrl_addr);
	
	/*关闭中断*/
	val|=(0x1U<<7);
	writel(val,intdis_addr);
	
	/*gpio方向设置，设置为输出模式*/
	val=readl(drim_addr);
	val|=(0x1U<<7);
	writel(val,drim_addr);
	
	/*输出使能*/
	val=readl(outen_addr);
	val|=(0x1U<<7);
	writel(val,outen_addr);
	
	/*默认关闭led*/
	val=readl(data_addr);
	val&=~(0x1U<<7);
	writel(val,data_addr);
	
	/*注册设备驱动*/
	/*申请设备号*/
	if(newchrled.major)
	{
		newchrled.devid=MKDEV(newchrled.major,0);
		ret=register_chrdev_region(newchrled.devid,NEWCHRLED_CNT,NEWCHRLED_NAME);
		if(ret)
		{
			goto out1;
		}
	}
	else
	{
		ret=alloc_chrdev_region(&newchrled.devid,0,NEWCHRLED_CNT,NEWCHRLED_NAME);
		if(ret)
		{
			goto out2;
		}
		newchrled.major=MAJOR(newchrled.devid);
		newchrled.minor=MINOR(newchrled.devid);
	}
	printk("newcheled major=%d,minor=%d\r\n",newchrled.major, newchrled.minor);
	
	/*初始化cdev*/
	newchrled.cdev.owner=THIS_MODULE;
	cdev_init(&newchrled.cdev,&fops);
	
	/*添加字符设备*/
	ret=cdev_add(&newchrled.cdev,newchrled.devid,NEWCHRLED_CNT);
	if(ret)
		goto out2;
	
	/*创建类*/
	newchrled.class=class_create(THIS_MODULE,NEWCHRLED_NAME);
	if(IS_ERR(newchrled.class))
	{
		ret=PTR_ERR(newchrled.class);
		goto out3;
	}
	/*创建设备*/
	newchrled.device=device_create(newchrled.class,NULL,newchrled.devid,NULL,NEWCHRLED_NAME);
	if(IS_ERR(newchrled.device))
	{
		ret=PTR_ERR(newchrled.device);
		goto out4;
	}
	return 0;
	
out4:
	class_destroy(newchrled.class);
out3:
	cdev_del(&newchrled.cdev);
out2:
	unregister_chrdev_region(newchrled.devid,NEWCHRLED_CNT);
out1:
	led_iounmap();
	
	
	return ret;
}

static void __exit newchrled_exit(void)
{
	/*注销设备*/
	device_destroy(newchrled.class,newchrled.devid);
	/*注销类*/
	class_destroy(newchrled.class);
	/*注销字符设备*/
	cdev_del(&newchrled.cdev);
	/*注销设备号*/
	unregister_chrdev_region(newchrled.devid,NEWCHRLED_CNT);
	/*取消地址映射*/
	led_iounmap();
}

module_init(newchrled_init);
module_exit(newchrled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHENGXZ");
MODULE_DESCRIPTION("NEW CHRDEV LED");



