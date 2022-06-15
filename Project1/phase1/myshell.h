/* $begin shellmain */
#include "csapp.h"
#define MAXARGS 128
#define blnk ' '
int argc = 0;

int builtin_command(char **);
int parseline(char*, char**);
void eval (char*);

/* Function prototypes */
void sig_childhandler(int sig){
	int status;
	pid_t id = waitpid(-1, &status, WNOHANG);
	return;
}

void Exeve(char path[MAXARGS], char *argv[MAXARGS], char **env)
{
    if (execve(path, argv, env) < 0)
    {
        if (execve(argv[0], argv, env) < 0)
        {
            printf("%s: Command not found.\n", argv[0]);
            exit(0);
        }
    }
}
/* $begin builtin_command */
/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
	// char msg[1024] = " / ";
	int return_val = -1;
	if (!strcmp(argv[0], "quit") || !strcmp(argv[0], "q") || !strcmp(argv[0], "exit")) /* quit command */
		exit(0);
	if (!strcmp(argv[0], "&")) /* Ignore singleton & */
		return 1;
	if (!strcmp(argv[0], "cd"))
	{
		const char *msg = "\t";
		if (argv[1] == NULL || strcmp(argv[1], "~") == 0)
			return_val = chdir(getenv("HOME"));
		else if (strcmp(argv[1], "-") == 0)
			return_val = chdir(getenv("OLDPWD"));
		else if (strcmp(argv[1], "-") == 0)
			return_val = chdir(getenv("PWD"));
		else if ((chdir(argv[1]) < 0))
		{
			fprintf(stderr, "%s: %s ", argv[0], argv[1]);
			perror(msg);
		}
		else if (argv[2] != NULL)
		{
			fprintf(stderr, "usage: cd [directory name] - you're in %s \n", argv[1]);
		}
		return 1;
	}
	return 0; /* Not a builtin command */
}
/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv)
{
	char *delim; /* Points to first space delimiter */
	int bg;		 /* Background job? */

	buf[strlen(buf) - 1] = blnk;	  /* Replace trailing '\n' with space */
	while (*buf && (*buf == blnk)) /* Ignore leading spaces */
		buf++;

	/* Build the argv list */
	argc = 0;
	while ((delim = strchr(buf, blnk)))
	{
		argv[argc++] = buf;
		// preprocess(buf, delim);
		char *temp = buf, *start = buf, *end = buf, flag = 0;

		while (*temp && (*temp != blnk)) // buf 부터 space가 아닌 영역를 탐색
		{
			if (*temp == '\'')
			{
				start = temp++;
				end = strchr(temp, '\'');
				temp = end;
				delim = strchr(temp, blnk);
				flag++;
				break;
			}
			if (*temp == '\"')
			{
				

				start = temp++;
				end = strchr(temp, '\"');
				temp = end;
				delim = strchr(temp, blnk);
				flag++;
				break;
			}
			temp++;
		}
		if (flag == 1)
		{
			temp = buf;
			while (temp <= delim)
			{
				if (buf > delim)
					*temp++ = blnk;
				else if (buf != start && buf != end)
					*temp++ = *buf; // buf가 따옴표(열고, 닫는)가 아니라면
				buf++;
			}
		}
		*delim = '\0';

		buf = delim + 1;
		while (*buf && (*buf == blnk)) /* Ignore spaces */
			buf++;
	}
	argv[argc] = NULL;

	if (argc == 0) /* Ignore blank line */
		return 1;

	/* Should the job run in the background? */
	if ((bg = (*argv[argc - 1] == '&')) != 0)
		argv[--argc] = NULL;

	return bg;
}
/* $end parseline */

/* $begin eval */
void eval(char *cmdline)
{
	char *argv[MAXARGS]; /* Argument list execve() */
	char buf[MAXLINE];	 /* Holds modified command line */
	int bg;				 /* Should the job run in bg or fg? */
	pid_t pid;			 /* Process id */
	strcpy(buf, cmdline); /* command string copy to buffer*/

	bg = parseline(buf, argv);
	if (argv[0] == NULL)
		return; /* Ignore empty lines */

	if (!builtin_command(argv))
	{
		char path[MAXARGS] = "/bin/";
		strcat(path, argv[0]); //
		if ((pid = Fork()) == 0)
		{ /* Child runs user job */
			if (pid < 0){
                unix_error("fork error");
            }
			Exeve(path, argv, environ);
		}

		/* Parent waits for foreground job to terminate */
		else
		{
			if (!bg) // foreground process
			{
				int status;
				if (waitpid(pid, &status, 0) < 0)
					unix_error("waitfg: waitpid error");
			}
			else // background process
				printf("%d %s", pid, cmdline);
		}
	}
	return;
}

/* $end eval */