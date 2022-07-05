/*#include <linux/types.h>
#include<linux/module.h>
#include<linux/cdev.h>
#include<linux/init.h>
#include<asm/io.h>
#include <asm/uaccess.h>
*/

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


#define LED_MAJOY 200          //主设备号
#define LED_NAME  "led"        //设备名字

/*
 *GPIO物理地址
 */
#define ZYNQ_GPIO_REG_BASH   0xE000A000
#define DATA_OFFSET          0x00000040
#define DRIM_OFFSET          0x00000204
#define OUTEN_OFFSET         0x00000208
#define INTDIS_OFFSET        0x00000214
#define APER_CLK_CTRL        0x0000012C

/*
 *映射后的GPIO地址
 */
static void __iomem *data_addr;
static void __iomem *drim_addr;
static void __iomem *outen_addr;
static void __iomem *intdis_addr;
static void __iomem *aper_clk_ctrl_addr;

static int led_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int ret;
	int val;
	char kern_buf[1];
	ret = copy_from_user(kern_buf,buf,cnt);
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

/* 设备操作函数 */
static struct file_operations led_fops={
	.owner=THIS_MODULE,
	.open=led_open,
	.read=led_read,
	.write=led_write,
	.release=led_release,	
};


/*
 *初始化
 */
static int __init led_init(void)
{
	int ret;
	u32 val;
	
	/*寄存器地址映射*/
	data_addr=ioremap(ZYNQ_GPIO_REG_BASH+DATA_OFFSET,4);
	drim_addr=ioremap(ZYNQ_GPIO_REG_BASH+DRIM_OFFSET,4);
	outen_addr=ioremap(ZYNQ_GPIO_REG_BASH+OUTEN_OFFSET,4);
	intdis_addr=ioremap(ZYNQ_GPIO_REG_BASH+INTDIS_OFFSET,4);
	aper_clk_ctrl_addr=ioremap(APER_CLK_CTRL,4);
	
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
		
	ret = register_chrdev(LED_MAJOY,LED_NAME,led_fops);
	if(0>ret)
	{
		printk(KERN_ERR"Register LED driver failed!\r\n");
		return ret;
		
	}
	return 0;
}

static void __exit led_exit(void)
{
	/*1、卸载设备 */
	unregister_chrdev(LED_MAJOY,LED_NAME);
	/*2、取消内存映射 */
	iounmap(data_addr);
	iounmap(drim_addr);
	iounmap(outen_addr);
	iounmap(intdis_addr);
	iounmap(aper_clk_ctrl_addr);
	
}

/*驱动函数入口和出口函数注册*/

module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("xuezhen zheng");
MODULE_DESCRIPTION("led device");
MODULE_LICENSE("GPL");
