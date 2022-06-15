/* $begin shellmain */
#include <errno.h>
#define MAXARGS 128
#define blnk ' '
int argc = 0;
/* Function prototypes */
void eval(char *cmdline);
void eval_pipeline(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void Remove(char *buf);
// pid_t pid;
/* $begin eval */
/* eval - Evaluate a command line */
void sig_child_handler(int sig)
{
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
void eval(char *cmdline)
{
    char *argv[MAXARGS];  /* Argument list execve() */
    char buf[MAXLINE];    /* Holds modified command line */
    int bg;               /* Should the job run in bg or fg? */
    pid_t pid;            /* Process id */
    strcpy(buf, cmdline); // string copy

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

/* $begin Remove */
void Remove(char *buf)
{
    while (*buf != '\0')
    {
        if (*buf == '\"' || *buf == '\'')
        {
            strcpy(buf, buf + 1);
            buf--;
        }

        buf++;
    }
}
/* $end Remove */

/* $begin parseline2 */
/* parseline2 - Parse the command line and build the argv array */
int parseline2(char *buf, char **argv)
{
    char *delim; /* Points to first space delimiter */
    int argc;    /* Number of args */
    int bg;      /* Background job? */

    buf[strlen(buf) - 1] = blnk;   /* Replace trailing '\n' with space */
    while (*buf && (*buf == blnk)) /* Ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, blnk)))
    {
        Remove(buf);
        argv[argc++] = buf;
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
void x(char *tmp[MAXARGS], int b)
{
    tmp[b] = NULL;
    b = 0;
}

void swapfd(int fd2[2], int fd1[2])
{
    fd2[1] = fd1[1];
    fd2[0] = fd1[0];
    return;
}
/* $end parseline */

void Exevp(char path[MAXARGS], char *temp[MAXARGS])
{
    if (execvp(path, temp) < 0)
    {
        if (execvp(temp[0], temp) < 0)
        {
            printf("%s: Command not found.\n", temp[0]);
            exit(0);
        }
    }
}
/* $begin eval_pipeline */
/* eval_pipeline - Evaluate a command line */
void eval_pipeline(char *cmdline)
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    strcpy(buf, cmdline); // string copy
    int fd1[2] = {0}, fd2[2] = {0};

    bg = parseline2(buf, argv);
    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    int a = 0, b = 0, c = 0;
    while (1)
    {
        char *temp[MAXARGS];
        for (; a < MAXARGS; a++)
        {
            if (argv[a] == NULL) /* if the command is the last */
            {
                x(temp, b);
                break;
            }
            else if (strcmp("|", argv[a]) == 0) /* if there exists a pipe in the command */
            {
                temp[b] = NULL;
                b = 0;
                a++;
                break;
            }
            else /* copy argv string into tmp*/
            {
                temp[b] = (char *)malloc(sizeof(char) * MAXLINE);
                int len = strlen(argv[a]);
                strncpy(temp[b], argv[a], len);
                b++;
            }
        }
        if (pipe(fd1) < 0)
        {
            perror("Pipe Error Ocurred");
        }

        if (!builtin_command(temp))
        {
            char path[MAXARGS] = "/bin/";
            strcat(path, argv[0]); // argv, path string concatenate
                                   ////////////////////////////////////////////////////////////////////////////////
            if ((pid = Fork()) == 0)
            {
                if (pid < 0)
                {
                    unix_error("fork error");
                }
                else if (pid == 0)
                { /* Child runs user job */

                    /* if the command is not last*/
                    if (argv[a] != NULL)
                    {
                        /* pipe */
                        dup2(fd1[1], STDOUT_FILENO);
                    }
                    /*  */
                    dup2(fd2[0], STDIN_FILENO);
                    close(fd1[0]);
                    strcat(path, temp[0]);
                    Exevp(path, temp);
                    
                }
            }
            // pid = fork();

            /* Parent waits for foreground job to terminate */
            else
            {
                if (!bg) /* foreground process (! background flag)*/
                {
                    int status;
                    if (waitpid(pid, &status, 0) < 0)
                        unix_error("waitfg: waitpid error");
                    close(fd1[1]);

                    // save in temporary array
                    swapfd(fd2, fd1);
                    
                }
                else
                    printf("%d %s", pid, cmdline);
            }
        }
        if (argv[a] == NULL)
            break;
    }
    return;
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
    int bg;      /* Background job? */

    buf[strlen(buf) - 1] = blnk;   /* Replace trailing '\n' with space */
    while (*buf && (*buf == blnk)) /* Ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, blnk)))
    {
        argv[argc++] = buf;
        // preprocess(buf, delim);
        char *temp = buf, *st = buf, *end = buf, flag = 0;

        while (*temp && (*temp != blnk)) // buf 부터 space가 아닌 영역를 탐색
        {
            if (*temp == '\"')
            {

                st = temp++;
                end = strchr(temp, '\"');
                // nonspace(delim,st,end,flag,temp);
                temp = end;
                delim = strchr(temp, blnk);
                flag++;
                break;
            }

            if (*temp == '\'')
            {
                st = temp++;
                end = strchr(temp, '\'');
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
                else if (buf != st && buf != end)
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
