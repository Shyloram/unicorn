/******************************************************************************
  Some simple Hisilicon Hi35xx system functions.

  Copyright (C), 2010-2015, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2015-6 Created
******************************************************************************/
#include "sys/types.h"
#include "sys/time.h"
#include "unistd.h"
#include "fcntl.h"
#include "sys/statfs.h"
#include "los_event.h"
#include "los_printf.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "eth_drv.h"
#include "arch/perf.h"
#include "fcntl.h"
#include "shell.h"
#include "stdio.h"
#include "hisoc/uart.h"
#include "fs/fs.h"
#include "shell.h"
#include "vfs_config.h"
#include "disk.h"
#include "los_cppsupport.h"
//#include "higmac.h"
#include "proc_fs.h"
#include "console.h"
#include "hirandom.h"
#include "nand.h"
#include "spi.h"
#include "dmac_ext.h"
#include "mtd_partition.h"
#include "limits.h"
#include "linux/fb.h"
#include "securec.h"
#include "implementation/usb_init.h"
#include "zmdconfig.h"

extern int mem_dev_register(void);
extern UINT32 osShellInit(char *);
extern void CatLogShell(void);
extern void hisi_eth_init(void);
//extern void tools_cmd_register(void);
extern void proc_fs_init(void);
extern int uart_dev_init(void);
extern int system_console_init(const char *);
extern int nand_init(void);
extern int add_mtd_partition( char *, UINT32 , UINT32 , UINT32 );
extern int spinor_init(void);
extern int spi_dev_init(void);
extern int i2c_dev_init(void);
extern int gpio_dev_init(void);
extern int dmac_init(void);
extern int ran_dev_register(void);
extern int mount(const char   *device_name,
              const char   *path,
              const char   *filesystemtype,
              unsigned long rwflag,
              const void   *data);
#ifdef CFG_SCATTER_FLAG
extern int mainloop_init(void);
extern int mainloop_start(void);
#else
extern int mainloop(void);
#endif
extern unsigned long __init_array_start__;
extern unsigned long __init_array_end__;

struct netif* pnetif;
extern void SDK_init(void);
extern void fuvc_frame_descriptors_set(void);

#ifdef TSET_ISP_TOOL
extern int pq_control_main(int argc, char **argv);
#endif

void print_time(const char* func, int line, const char* desc)
{
    if (desc == 0)
    {
        start_printf("\n\033[0;31m %s:%d: %dms \n\033[0m", func, line, hi_getmsclock());
    }
    else
    {
        start_printf("\n\033[0;31m %s:%d: %dms [%s] \n\033[0m", func, line, hi_getmsclock(), desc);
    }
}

static int image_read(void* memaddr, unsigned long start, unsigned long size)
{
	return hispinor_read(memaddr, start, size);
}

int secure_func_register(void)
{
	int ret;
	STlwIPSecFuncSsp stlwIPSspCbk= {0};
	stlwIPSspCbk.pfMemset_s = memset_s;
	stlwIPSspCbk.pfMemcpy_s = memcpy_s;
	stlwIPSspCbk.pfStrNCpy_s = strncpy_s;
	stlwIPSspCbk.pfStrNCat_s = strncat_s;
	stlwIPSspCbk.pfStrCat_s = strcat_s;
	stlwIPSspCbk.pfMemMove_s = memmove_s;
	stlwIPSspCbk.pfSnprintf_s = snprintf_s;
	stlwIPSspCbk.pfRand = rand;
	ret = lwIPRegSecSspCbk(&stlwIPSspCbk);
	if (ret != 0)
	{
		PRINT_ERR("\n***lwIPRegSecSspCbk Failed***\n");
		return -1;
	}

	return ret;
}

void net_init(void)
{
	(void)secure_func_register();
	tcpip_init(NULL, NULL);
#ifndef ZMD_APP_WIFI
#ifdef LOSCFG_DRIVERS_HIGMAC
	extern struct los_eth_driver higmac_drv_sc;
	pnetif = &(higmac_drv_sc.ac_if);
	higmac_init();
#endif

#ifdef LOSCFG_DRIVERS_HIETH_SF
	extern struct los_eth_driver hisi_eth_drv_sc;
	pnetif = &(hisi_eth_drv_sc.ac_if);
	hisi_eth_init();
#endif

	dprintf("cmd_startnetwork : DHCP_BOUND finished\n");
	netif_set_up(pnetif);
#endif
}

extern char shell_working_directory[PATH_MAX];
extern unsigned int g_uwFatSectorsPerBlock;
extern unsigned int g_uwFatBlockNums;
extern UINT32 g_sys_mem_addr_end;
extern unsigned int g_uart_fputc_en;
void board_config(void)
{
    g_sys_mem_addr_end = SYS_MEM_BASE + SYS_MEM_SIZE_DEFAULT;
    g_uwSysClock = OS_SYS_CLOCK;
    g_uart_fputc_en = 1;
    extern unsigned long g_usb_mem_addr_start;
    g_usb_mem_addr_start = g_sys_mem_addr_end;

    g_uwFatSectorsPerBlock = CONFIG_FS_FAT_SECTOR_PER_BLOCK;
    g_uwFatBlockNums       = CONFIG_FS_FAT_BLOCK_NUMS;
}

void misc_driver_init(void)
{
	print_time("driver_init", __LINE__, "start");
    ran_dev_register();
    spinor_init();
    spi_dev_init();
    i2c_dev_init();
    gpio_dev_init();
    dmac_init();
	mem_dev_register();
  	int value= 0x0000;
	writel(value,  0x200F009C); //dolphin 180621 forbid sdio-0
	SD_MMC_Host_init();
}

void app_init(void)
{
	misc_driver_init();
	print_time(__FUNCTION__, __LINE__, "end drv init");

	SDK_init();
	print_time(__FUNCTION__, __LINE__, "end sdk init");

    uart_dev_init();
	system_console_init(TTY_DEVICE);

#ifdef CFG_SCATTER_FLAG
	//support c++
	LOS_CppSystemInit((unsigned long)&__init_array_start__, (unsigned long)&__init_array_end__,BEFORE_SCATTER);

	print_time(__FUNCTION__, __LINE__ ,"mainloop  init");
	mainloop_init();
	
	dprintf("------------------before scatter------------------\n");
	LOS_ScatterLoad(0x100000, image_read, 1);//"0x100000" is the address where the app is burned. "1" is spi flash  read unit
	dprintf("------------------after scatter------------------\n");
#endif

#ifndef CFG_FAST_IMAGE
	
    osShellInit(TTY_DEVICE);
#ifndef ZMD_APP_WIFI
	net_init();
#endif

	print_time(__FUNCTION__, __LINE__ ,"mainloop  start");
#ifdef CFG_SCATTER_FLAG
	//support c++
	LOS_CppSystemInit((unsigned long)&__init_array_start__, (unsigned long)&__init_array_end__,AFTER_SCATTER);
	mainloop_start();
#else
	//support c++
	LOS_CppSystemInit((unsigned long)&__init_array_start__, (unsigned long)&__init_array_end__,NO_SCATTER);
	mainloop();
#endif

	//last load
	proc_fs_init();
    //tools_cmd_register();
	CatLogShell();

#ifdef TSET_ISP_TOOL
	sleep(4);

	UINT32 stream_taskid;
    TSK_INIT_PARAM_S stappTask;
	memset(&stappTask, 0, sizeof(TSK_INIT_PARAM_S));
    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)pq_control_main;
    stappTask.uwStackSize  = 0x10000;
    stappTask.pcName = "pq_control_main";
    stappTask.usTaskPrio = 10;
    stappTask.uwResved   = LOS_TASK_STATUS_DETACHED;
    LOS_TaskCreate(&stream_taskid, &stappTask);
    dprintf("pq_control_main\n");
#endif
	
#endif
	while(1)
	{
		sleep(1);
	}
}
