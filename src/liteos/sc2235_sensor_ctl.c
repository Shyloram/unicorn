#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "sys/time.h"

#include "hi_comm_video.h"

#ifdef HI_GPIO_I2C
#include "gpioi2c_ex.h"
#else
#include "hi_i2c.h"
#endif

const unsigned char sensor_i2c_addr	    =	0x60;		/* I2C Address of SC2135 */
const unsigned int  sensor_addr_byte	=	2;
const unsigned int  sensor_data_byte	=	1;
static int g_fd = -1;
static int flag_init = 0;

extern WDR_MODE_E genSensorMode;
extern HI_U8 gu8SensorImageMode;
extern HI_BOOL bSensorInit;

int sensor_i2c_init(void)
{
    if(g_fd >= 0)
    {
        return 0;
    }
#ifdef HI_GPIO_I2C
    int ret;

    g_fd = open("/dev/gpioi2c_ex", 0);
    if(g_fd < 0)
    {
        printf("Open gpioi2c_ex error!\n");
        return -1;
    }
#else
    int ret;

    g_fd = open("/dev/i2c-0", O_RDWR);
    if(g_fd < 0)
    {
        printf("Open /dev/i2c-0 error!\n");
        return -1;
    }

    ret = ioctl(g_fd, I2C_SLAVE_FORCE, sensor_i2c_addr);
    if (ret < 0)
    {
        printf("CMD_SET_DEV error!\n");
        return ret;
    }
#endif

    return 0;
}

int sensor_i2c_exit(void)
{
    if (g_fd >= 0)
    {
        close(g_fd);
        g_fd = -1;
        return 0;
    }
    return -1;
}


int sensor_read_register(int addr)

{
	int idx = 0;
    int ret = 0, data = 0;
    char buf[8] = {0};
    buf[idx++] = addr & 0xFF;

    if (sensor_addr_byte == 2)
    {
        ret = ioctl(g_fd, I2C_16BIT_REG, 1);
        buf[idx++] = (addr >> 8);
    }
    else
    {
        ret = ioctl(g_fd, I2C_16BIT_REG, 0);
    }
    if (ret < 0)
    {
        printf("CMD_SET_REG_WIDTH error!\n");
        return -1;
    }

    buf[idx++] = data;
    if (sensor_data_byte == 2)
    {
        ret = ioctl(g_fd, I2C_16BIT_DATA, 1);
        buf[idx++] = data >> 8;
    }
    else
    {
        ret = ioctl(g_fd, I2C_16BIT_DATA, 0);
    }
    if (ret)
    {
        printf("hi_i2c write faild!\n");
        return -1;
    }
    ret = read(g_fd, buf, idx);

    return buf[0];
}


int sensor_write_register(int addr, int data)
{
#ifdef HI_GPIO_I2C
    i2c_data.dev_addr = sensor_i2c_addr;
    i2c_data.reg_addr = addr;
    i2c_data.addr_byte_num = sensor_addr_byte;
    i2c_data.data = data;
    i2c_data.data_byte_num = sensor_data_byte;

    ret = ioctl(g_fd, GPIO_I2C_WRITE, &i2c_data);

    if (ret)
    {
        printf("GPIO-I2C write faild!\n");
        return ret;
    }
#else
    if(flag_init == 0)
    {

		sensor_i2c_init();
		flag_init = 1;
    }

    int idx = 0;
    int ret;
    char buf[8];

    buf[idx++] = addr & 0xFF;
    if (sensor_addr_byte == 2)
    {
    	ret = ioctl(g_fd, I2C_16BIT_REG, 1);
        buf[idx++] = addr >> 8;
    }
    else
    {
    	ret = ioctl(g_fd, I2C_16BIT_REG, 0);
    }

    if (ret < 0)
    {
        printf("CMD_SET_REG_WIDTH error!\n");
        return -1;
    }

    buf[idx++] = data;
    if (sensor_data_byte == 2)
    {
    	ret = ioctl(g_fd, I2C_16BIT_DATA, 1);
        buf[idx++] = data >> 8;
    }
    else
    {
    	ret = ioctl(g_fd, I2C_16BIT_DATA, 0);
    }

    if (ret)
    {
        printf("hi_i2c write faild!\n");
        return -1;
    }

    ret = write(g_fd, buf, idx);
    if(ret < 0)
    {
    	printf("I2C_WRITE error!\n");
    	return -1;
    }

    //printf("%#x, %#x, \n",data,sensor_read_register(addr));
#endif
	return 0;
}

static void delay_ms(int ms) {
    usleep(ms*1000);
}

void sensor_prog(int* rom)
{
    int i = 0;
    while (1) {
        int lookup = rom[i++];
        int addr = (lookup >> 16) & 0xFFFF;
        int data = lookup & 0xFFFF;
        if (addr == 0xFFFE) {
            delay_ms(data);
        } else if (addr == 0xFFFF) {
            return;
        } else {
            sensor_write_register(addr, data);
        }
    }
}

void sc2235_1080p20fps_init();

#define SENSOR_1080P_20FPS_MODE  (1)

void sensor_init()
{

    sensor_i2c_init();
	
    sc2235_1080p20fps_init();

    return ;
}

void sensor_exit()
{
    sensor_i2c_exit();
	flag_init = 0;
    return;
}

unsigned short int g_mirrorflip_data = 0;

void sc2235_1080p20fps_init()
{
/*
    g_mirrorflip_data = sensor_read_register(0x3221);

	sensor_write_register(0x0103,0x01);  // reset all registers
	usleep(1000);
	sensor_write_register(0x0100,0x00);  // stream output hold
	
	sensor_write_register(0x3621,0x28); 
	
	sensor_write_register(0x3309,0x60);
	sensor_write_register(0x331f,0x4d);
	//usleep(1000);
	sensor_write_register(0x3321,0x4f);
	sensor_write_register(0x33b5,0x10);
	sensor_write_register(0x3303,0x20);
	sensor_write_register(0x331e,0x0d);
	//usleep(1000);
	sensor_write_register(0x3320,0x0f);
	sensor_write_register(0x3622,0x02);
	sensor_write_register(0x3633,0x42);
	sensor_write_register(0x3634,0x42);
	//usleep(1000);
	sensor_write_register(0x3306,0x66);
	sensor_write_register(0x330b,0xd1);
	sensor_write_register(0x3301,0x0e);
	sensor_write_register(0x320c,0x0a);
	//usleep(1000);
	sensor_write_register(0x320d,0x50);
	sensor_write_register(0x3364,0x05);
	sensor_write_register(0x363c,0x28);
	sensor_write_register(0x363b,0x0a);
	//usleep(1000);
	sensor_write_register(0x3635,0xa0);
	sensor_write_register(0x4500,0x59);
	sensor_write_register(0x3d08,0x00);
	sensor_write_register(0x3908,0x11);
	//usleep(1000);
	sensor_write_register(0x363c,0x08);
	sensor_write_register(0x3e03,0x03);
	sensor_write_register(0x3e01,0x46);
	sensor_write_register(0x3381,0x0a);
	//usleep(1000);
	sensor_write_register(0x3348,0x09);
	sensor_write_register(0x3349,0x50);
	sensor_write_register(0x334a,0x02);
	sensor_write_register(0x334b,0x60);
	//usleep(1000);
	sensor_write_register(0x3380,0x04);
	sensor_write_register(0x3340,0x06);
	sensor_write_register(0x3341,0x50); 
	sensor_write_register(0x3342,0x02);
	//usleep(1000);
	sensor_write_register(0x3343,0x60);
	sensor_write_register(0x3632,0x88);
	sensor_write_register(0x3309,0xa0); 
	sensor_write_register(0x331f,0x8d);
	//usleep(1000);
	sensor_write_register(0x3321,0x8f);
	sensor_write_register(0x335e,0x01);
	sensor_write_register(0x335f,0x03);
	sensor_write_register(0x337c,0x04);
	//usleep(1000);
	sensor_write_register(0x337d,0x06);
	sensor_write_register(0x33a0,0x05);
	sensor_write_register(0x3301,0x05);
	sensor_write_register(0x3670,0x08);
	//usleep(1000);
	sensor_write_register(0x367e,0x07);
	sensor_write_register(0x367f,0x0f);
	sensor_write_register(0x3677,0x2f);
	sensor_write_register(0x3678,0x22);
	//usleep(1000);
	sensor_write_register(0x3679,0x42);
	sensor_write_register(0x337f,0x03);
	sensor_write_register(0x3368,0x02);
	sensor_write_register(0x3369,0x00);
	//usleep(1000);
	sensor_write_register(0x336a,0x00);
	sensor_write_register(0x336b,0x00);
	sensor_write_register(0x3367,0x08);
	sensor_write_register(0x330e,0x30);
	//usleep(1000);
	sensor_write_register(0x3366,0x7c);
	sensor_write_register(0x3635,0xc1);
	sensor_write_register(0x363b,0x09);
	sensor_write_register(0x363c,0x07);
	//usleep(1000);
	sensor_write_register(0x391e,0x00);
	sensor_write_register(0x3637,0x14);
	sensor_write_register(0x3306,0x54);
	sensor_write_register(0x330b,0xd8);
	//usleep(1000);
	sensor_write_register(0x366e,0x08);
	sensor_write_register(0x366f,0x2f);
	sensor_write_register(0x3631,0x84);
	sensor_write_register(0x3630,0x48);
	sensor_write_register(0x3622,0x06);
	//usleep(1000);
	sensor_write_register(0x3638,0x1f);
	sensor_write_register(0x3625,0x02);
	sensor_write_register(0x3636,0x24);
	sensor_write_register(0x3348,0x08);
	//usleep(1000);
	sensor_write_register(0x3e03,0x0b);
	sensor_write_register(0x3342,0x03);
	sensor_write_register(0x3343,0xa0);
	sensor_write_register(0x334a,0x03);
	//usleep(1000);
	sensor_write_register(0x334b,0xa0);
	sensor_write_register(0x3343,0xb0);
	sensor_write_register(0x334b,0xb0);
	sensor_write_register(0x3679,0x43);
	//usleep(1000);
	sensor_write_register(0x3802,0x01);
	sensor_write_register(0x3235,0x04);	
	sensor_write_register(0x3236,0x63);
	sensor_write_register(0x3343,0xd0);
	//usleep(1000);
	sensor_write_register(0x334b,0xd0);
	sensor_write_register(0x3348,0x07);
	sensor_write_register(0x3349,0x80);

	sensor_write_register(0x391b,0x4d);	
	//usleep(1000);
	sensor_write_register(0x320c,0x08);
	sensor_write_register(0x320d,0x98);	

	sensor_write_register(0x3039,0x33);
	sensor_write_register(0x3034,0x07);
	//usleep(1000);	
	sensor_write_register(0x330a,0x01);
	sensor_write_register(0x330b,0x78);
	sensor_write_register(0x3306,0x70);
	//usleep(1000);
//20170818
	sensor_write_register(0x334a,0x04); 
	sensor_write_register(0x334b,0x20);
	sensor_write_register(0x3342,0x04); 
	sensor_write_register(0x3343,0x20);
	//usleep(1000);
	sensor_write_register(0x3034,0x03);

	sensor_write_register(0x3306,0x46);
	sensor_write_register(0x330a,0x00);
	sensor_write_register(0x330b,0xcc);
	sensor_write_register(0x3639,0x09);
	//usleep(1000);
	//0822
	sensor_write_register(0x3901,0x02);
	sensor_write_register(0x3222,0x29);

    sensor_write_register(0x3678,0x42); 
	sensor_write_register(0x3302,0x34); 
	//usleep(1000);
	//0824
	sensor_write_register(0x3670,0x00);
	sensor_write_register(0x3313,0x05);

	sensor_write_register(0x5780,0xff);
	sensor_write_register(0x5781,0x04);
	//usleep(1000);	
	sensor_write_register(0x5785,0x18);
	sensor_write_register(0x391b,0x4f);

	sensor_write_register(0x3e03,0x0b);
	sensor_write_register(0x3802,0x0); //20170830 globe hold Á¢¼´ÉúÐ§

    sensor_write_register(0x3221, g_mirrorflip_data);
	//usleep(1000);
	sensor_write_register(0x0100,0x01);
	
	sensor_write_register(0x3d08,0x01);
*/  
	g_mirrorflip_data = sensor_read_register(0x3221);
  	sensor_write_register(0x0103,0x01);  // reset all registers
	sensor_write_register(0x0100,0x00);  // stream output hold
	sensor_write_register(0x3034,0x03);
	sensor_write_register(0x3039,0x33);
	sensor_write_register(0x320c,0x08);
	sensor_write_register(0x320d,0x98);
    sensor_write_register(0x3221, g_mirrorflip_data);
	sensor_write_register(0x3222,0x29);
	sensor_write_register(0x3235,0x04);
	sensor_write_register(0x3236,0x63);
	sensor_write_register(0x3301,0x05);
	sensor_write_register(0x3302,0x34);
	sensor_write_register(0x3303,0x20);
	sensor_write_register(0x3306,0x46);
	sensor_write_register(0x3309,0xa0);
	sensor_write_register(0x330a,0x00);
	sensor_write_register(0x330b,0xcc);
	sensor_write_register(0x330e,0x30);
	sensor_write_register(0x3313,0x05);
	sensor_write_register(0x331e,0x0d);
	sensor_write_register(0x331f,0x8d);
	sensor_write_register(0x3320,0x0f);
	sensor_write_register(0x3321,0x8f);
	sensor_write_register(0x3340,0x06);
	sensor_write_register(0x3341,0x50);
	sensor_write_register(0x3342,0x04);
	sensor_write_register(0x3343,0x20);
	sensor_write_register(0x3348,0x07);
	sensor_write_register(0x3349,0x80);
	sensor_write_register(0x334a,0x04);
	sensor_write_register(0x334b,0x20);
	sensor_write_register(0x335e,0x01);
	sensor_write_register(0x335f,0x03);
	sensor_write_register(0x3364,0x05);
	sensor_write_register(0x3366,0x7c);
	sensor_write_register(0x3367,0x08);
	sensor_write_register(0x3368,0x02);
	sensor_write_register(0x3369,0x00);
	sensor_write_register(0x336a,0x00);
	sensor_write_register(0x336b,0x00);
	sensor_write_register(0x337c,0x04);
	sensor_write_register(0x337d,0x06);
	sensor_write_register(0x337f,0x03);
	sensor_write_register(0x3380,0x04);
	sensor_write_register(0x3381,0x0a);
	sensor_write_register(0x33a0,0x05);
	sensor_write_register(0x33b5,0x10);
	sensor_write_register(0x3621,0x28);
	sensor_write_register(0x3622,0x06);
	sensor_write_register(0x3625,0x02);
	sensor_write_register(0x3630,0x48);
	sensor_write_register(0x3631,0x84);
	sensor_write_register(0x3632,0x88);
	sensor_write_register(0x3633,0x42);
	sensor_write_register(0x3634,0x42);
	sensor_write_register(0x3635,0xc1);
	sensor_write_register(0x3636,0x24);
	sensor_write_register(0x3637,0x14);
	sensor_write_register(0x3638,0x1f);
	sensor_write_register(0x3639,0x09);
	sensor_write_register(0x363b,0x09);
	sensor_write_register(0x363c,0x07);
	sensor_write_register(0x366e,0x08);
	sensor_write_register(0x366f,0x2f);
	sensor_write_register(0x3670,0x00);
	sensor_write_register(0x3677,0x2f);
	sensor_write_register(0x3678,0x42);
	sensor_write_register(0x3679,0x43);
	sensor_write_register(0x367e,0x07);
	sensor_write_register(0x367f,0x0f);
	sensor_write_register(0x3802,0x00);
	sensor_write_register(0x3901,0x02);
	sensor_write_register(0x3908,0x11);
	sensor_write_register(0x391b,0x4f);
	sensor_write_register(0x391e,0x00);
	sensor_write_register(0x3e01,0x46);
	sensor_write_register(0x3e03,0x0b);
	sensor_write_register(0x4500,0x59);
	sensor_write_register(0x5780,0xff);
	sensor_write_register(0x5781,0x04);
	sensor_write_register(0x5785,0x18);
	sensor_write_register(0x3d08,0x01);
	sensor_write_register(0x0100,0x01);  // stream output on	
	bSensorInit = HI_TRUE;

    return;
}
