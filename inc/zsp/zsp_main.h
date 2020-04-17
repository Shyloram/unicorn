#ifndef __ZSP_MAIN_H__
#define __ZSP_MAIN_H__

extern int zsp_main(void);
extern void zsp_test_command(void);
extern int zsp_send_h264(char *buf, int len, struct timeval * p_timeout, unsigned long long pts);



#endif  /* __ZSP_MAIN_H__ */
