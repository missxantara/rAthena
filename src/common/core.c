// $Id: core.c,v 1.1.1.1 2004/09/10 17:44:49 MagicalTux Exp $
// original : core.c 2003/02/26 18:03:12 Rev 1.7

#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <signal.h>
#include <string.h>
#ifdef DUMPSTACK
	#ifndef _WIN32	// HAVE_EXECINFO_H
		#include <execinfo.h>
	#endif
#endif

#include "../common/mmo.h"
#include "malloc.h"
#include "core.h"
#include "socket.h"
#include "timer.h"
#include "version.h"
#include "showmsg.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static void (*term_func)(void)=NULL;

/*======================================
 *	CORE : Set function
 *--------------------------------------
 */
void set_termfunc(void (*termfunc)(void))
{
	term_func = termfunc;
}

// Added by Gabuzomeu
//
// This is an implementation of signal() using sigaction() for portability.
// (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
// Programming in the UNIX Environment_.
//
#ifndef SIGPIPE
#define SIGPIPE SIGINT
#endif

#ifndef POSIX
#define compat_signal(signo, func) signal(signo, func)
#else
sigfunc *compat_signal(int signo, sigfunc *func)
{
  struct sigaction sact, oact;

  sact.sa_handler = func;
  sigemptyset(&sact.sa_mask);
  sact.sa_flags = 0;
#ifdef SA_INTERRUPT
  sact.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

  if (sigaction(signo, &sact, &oact) < 0)
    return (SIG_ERR);

  return (oact.sa_handler);
}
#endif

/*======================================
 *	CORE : Signal Sub Function
 *--------------------------------------
 */

static void sig_proc(int sn)
{
	int i;
	static int is_called = 0;

	if(is_called++)
		return;

	switch(sn){
	case SIGINT:
	case SIGTERM:
		if(term_func)
			term_func();
		for(i=0;i<fd_max;i++){
			if(!session[i])
				continue;
			close(i);
		}
		exit(0);
		break;
	}
}

/*=========================================
 *	Dumps the stack using glibc's backtrace
 *-----------------------------------------
 */
static void sig_dump(int sn)
{
	#ifdef DUMPSTACK
		FILE *fp;
		void* array[20];

		char **stack;
		size_t size;		
		int no = 0;
		char tmp[256];

		// search for a usable filename
		do {
			sprintf(tmp,"save/stackdump_%04d.txt", ++no);
		} while((fp = fopen(tmp,"r")) && (fclose(fp), no < 9999));
		// dump the trace into the file
		if ((fp = fopen (tmp,"w")) != NULL) {

			fprintf(fp,"Exception: %s\n", strsignal(sn));
			fprintf(fp,"Stack trace:\n");
			size = backtrace (array, 20);
			stack = backtrace_symbols (array, size);

			for (no = 0; no < size; no++) {

				fprintf(fp, "%s\n", stack[no]);

			}
			fprintf(fp,"End of stack trace\n");

			fclose(fp);
			free(stack);
		}
	#endif
	// When pass the signal to the system's default handler
	compat_signal(sn, SIG_DFL);
	raise(sn);
}

int get_svn_revision(char *svnentry) { // Warning: minor syntax checking
	char line[1024];
	int rev = 0;
	FILE *fp;
	if ((fp = fopen(svnentry, "r")) == NULL) {
		return 0;
	} else {
		while (fgets(line,1023,fp)) if (strstr(line,"revision=")) break;
		fclose(fp);
		if (sscanf(line," %*[^\"]\"%d%*[^\n]",&rev)==1) 
		return rev;
		else
		return 0;
	}
//	return 0;
}

/*======================================
 *	CORE : Display title
 *--------------------------------------
 */

static void display_title(void)
{
	int revision;
	// for help with the console colors look here:
	// http://www.edoceo.com/liberum/?doc=printf-with-color
	// some code explanation (used here):
	// \033[2J : clear screen and go up/left (0, 0 position)
	// \033[K  : clear line from actual position to end of the line
	// \033[0m : reset color parameter
	// \033[1m : use bold for font
	printf("\033[2J"); // clear screen and go up/left (0, 0 position in text)
	printf("\033[37;44m          (=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=)\033[K\033[0m\n"); // white writing (37) on blue background (44), \033[K clean until end of file
	printf("\033[0;44m          (\033[1;33m        (c)2005 eAthena Development Team presents        \033[0;44m)\033[K\033[0m\n"); // yellow writing (33)
	printf("\033[0;44m          (\033[1m       ______  __    __                                  \033[0;44m)\033[K\033[0m\n"); // 1: bold char, 0: normal char
	printf("\033[0;44m          (\033[1m      /\\  _  \\/\\ \\__/\\ \\                     v%2d.%02d.%02d   \033[0;44m)\033[K\033[0m\n", ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION); // 1: bold char, 0: normal char
	printf("\033[0;44m          (\033[1m    __\\ \\ \\_\\ \\ \\ ,_\\ \\ \\___      __    ___      __      \033[0;44m)\033[K\033[0m\n"); // 1: bold char, 0: normal char
	printf("\033[0;44m          (\033[1m  /'__`\\ \\  __ \\ \\ \\/\\ \\  _ `\\  /'__`\\/' _ `\\  /'__`\\    \033[0;44m)\033[K\033[0m\n"); // 1: bold char, 0: normal char
	printf("\033[0;44m          (\033[1m /\\  __/\\ \\ \\/\\ \\ \\ \\_\\ \\ \\ \\ \\/\\  __//\\ \\/\\ \\/\\ \\_\\.\\_  \033[0;44m)\033[K\033[0m\n"); // 1: bold char, 0: normal char
	printf("\033[0;44m          (\033[1m \\ \\____\\\\ \\_\\ \\_\\ \\__\\\\ \\_\\ \\_\\ \\____\\ \\_\\ \\_\\ \\__/.\\_\\ \033[0;44m)\033[K\033[0m\n"); // 1: bold char, 0: normal char
	printf("\033[0;44m          (\033[1m  \\/____/ \\/_/\\/_/\\/__/ \\/_/\\/_/\\/____/\\/_/\\/_/\\/__/\\/_/ \033[0;44m)\033[K\033[0m\n"); // 1: bold char, 0: normal char
	printf("\033[0;44m          (\033[1m   _   _   _   _   _   _   _     _   _   _   _   _   _   \033[0;44m)\033[K\033[0m\n"); // 1: bold char, 0: normal char
	printf("\033[0;44m          (\033[1m  / \\ / \\ / \\ / \\ / \\ / \\ / \\   / \\ / \\ / \\ / \\ / \\ / \\  \033[0;44m)\033[K\033[0m\n"); // 1: bold char, 0: normal char
	printf("\033[0;44m          (\033[1m ( e | n | g | l | i | s | h ) ( A | t | h | e | n | a ) \033[0;44m)\033[K\033[0m\n"); // 1: bold char, 0: normal char
	printf("\033[0;44m          (\033[1m  \\_/ \\_/ \\_/ \\_/ \\_/ \\_/ \\_/   \\_/ \\_/ \\_/ \\_/ \\_/ \\_/  \033[0;44m)\033[K\033[0m\n"); // 1: bold char, 0: normal char
	printf("\033[0;44m          (\033[1m                                                         \033[0;44m)\033[K\033[0m\n"); // yellow writing (33)
	printf("\033[0;44m          (\033[1;33m  Advanced Fusion Maps (c) 2003-2005 The Fusion Project  \033[0;44m)\033[K\033[0m\n"); // yellow writing (33)
	printf("\033[37;44m          (=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=)\033[K\033[0m\n\n"); // reset color
	
	if ((revision = get_svn_revision(".svn\\entries"))>0) {
		snprintf(tmp_output,sizeof(tmp_output),"SVN Revision: '"CL_WHITE"%d"CL_RESET"'.\n",revision);
		ShowInfo(tmp_output);
	}
}

/*======================================
 *	CORE : MAINROUTINE
 *--------------------------------------
 */

int runflag = 1;
char pid_file[256];

void pid_delete(void) {
	unlink(pid_file);
}

void pid_create(const char* file) {
	FILE *fp;
	int len = strlen(file);
	strcpy(pid_file,file);
	if(len > 4 && pid_file[len - 4] == '.') {
		pid_file[len - 4] = 0;
	}
	strcat(pid_file,".pid");
	fp = fopen(pid_file,"w");
	if(fp) {
#ifdef _WIN32
		fprintf(fp,"%d",GetCurrentProcessId());
#else
		fprintf(fp,"%d",getpid());
#endif
		fclose(fp);
		atexit(pid_delete);
	}
}

int main(int argc,char **argv)
{
	int next;

	pid_create(argv[0]);
	Net_Init();
	do_socket();
	
	compat_signal(SIGPIPE,SIG_IGN);
	compat_signal(SIGTERM,sig_proc);
	compat_signal(SIGINT,sig_proc);

	// Signal to create coredumps by system when necessary (crash)
	compat_signal(SIGSEGV, sig_dump);
	compat_signal(SIGFPE, sig_dump);
	compat_signal(SIGILL, sig_dump);
	#ifndef _WIN32
		compat_signal(SIGBUS, sig_dump);
		compat_signal(SIGTRAP, SIG_DFL);
	#endif

	display_title();

	do_init_memmgr(argv[0]); // 一番最初に実行する必要がある

	tick_ = time(0);

	do_init(argc,argv);
	while(runflag){
		next=do_timer(gettick_nocache());
		do_sendrecv(next);
		do_parsepacket();
	}

	return 0;
}
