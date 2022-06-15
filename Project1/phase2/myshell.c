#include "csapp.h"
#include "myshell.h"
#define MAXARGS   128

int main() 
{
	char cmdline[MAXLINE]; /* Command line */
	// Signal(SIGCHLD, sig_child_handler);	
	// Signal(SIGINT, sig_int_handler);
	// Signal(SIGTSTP, sig_stop_handler);

	while (1) {
		/* Read */
		// signal_initialize;
		printf("CSE4100-SP-P#1> ");   
		                
		char *tmp = Fgets(cmdline, MAXLINE, stdin); 
		if (feof(stdin))
			exit(0);

		/* Evaluate */
		// eval(cmdline);
		if (!strstr(cmdline, "|")) eval(cmdline);
        else eval_pipeline(cmdline);
	} 
}
