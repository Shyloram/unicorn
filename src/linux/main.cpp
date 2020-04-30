#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "modinterface.h"

/*进程与线程信号处理函数*/
void SignalHandler(int signum)
{
	if(signum == 11)
	sleep(3);
	
//DEBUG
	unsigned int pid = getpid();
	unsigned long int tid = pthread_self();
  	int ppid = getppid();
  	printf("%s %d %s tid:[%lu] pid:[%d] ppid:[%d] Receive Signal %s  \n", __FILE__,__LINE__,__FUNCTION__,tid,pid,ppid,strsignal(signum));

	switch( signum)
	{
		case SIGCHLD:
			break;
			
		case SIGSEGV:
			break;
			
		case SIGILL:	
		case SIGFPE:
			break;
				
		case SIGINT:
		case SIGTERM:
		case SIGKILL:	
			stopmodinterface();
			exit(0);
			break;
			
		case SIGABRT:
		case SIGSTOP:
			exit(0);
			break;

		default:
			break;	
	}
}

void SignalRegister()
{
	//系统信号处理
	signal(SIGINT,SignalHandler);	
	signal(SIGQUIT,SignalHandler);
	signal(SIGABRT,SignalHandler );
	signal(SIGFPE,SignalHandler );
	signal(SIGILL,SignalHandler);
	signal(SIGKILL,SignalHandler);
	signal(SIGSEGV,SignalHandler);
	signal(SIGSEGV,SignalHandler);
	signal(SIGPIPE,SignalHandler);
	signal(SIGTERM,SignalHandler );
	signal(SIGCHLD,SignalHandler);
	signal(SIGSTOP,SignalHandler);
	signal(SIGUSR1,SignalHandler);	
	signal(SIGUSR2,SignalHandler);
/*
       SIGHUP        1       Term    Hangup detected on controlling terminal
                                     or death of controlling process
       SIGINT        2       Term    Interrupt from keyboard
       
       SIGQUIT       3       Core    Quit from keyboard
       SIGILL        4       Core    Illegal Instruction
       SIGABRT       6       Core    Abort signal from abort(3)
       SIGFPE        8       Core    Floating point exception//
       SIGKILL       9       Term    Kill signal
       SIGSEGV      11       Core    Invalid memory reference
       SIGPIPE      13       Term    Broken pipe: write to pipe with no readers
       SIGALRM      14       Term    Timer signal from alarm(2)
       SIGTERM      15       Term    Termination signal
       SIGUSR1   30,10,16    Term    User-defined signal 1
       SIGUSR2   31,12,17    Term    User-defined signal 2
       SIGCHLD   20,17,18    Ign     Child stopped or terminated
       SIGCONT   19,18,25            Continue if stopped
       SIGSTOP   17,19,23    Stop    Stop process
       SIGTSTP   18,20,24    Stop    Stop typed at tty
       SIGTTIN   21,21,26    Stop    tty input for background process
       SIGTTOU   22,22,27    Stop    tty output for background process	
*/  
}

int main(void)
{
	SignalRegister();
	startmodinterface();
	while(1)sleep(1000);
}
