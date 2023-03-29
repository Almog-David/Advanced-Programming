#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <stdbool.h>
#include <string.h>
#include <signal.h>

char command[1024];
char copy_command[1024];
char *history[20];
char *token;
char *outfile;
bool amper, redirect, piping, retid, outerr, concat;
int i, status, argc, fd, history_index, numOfPipes = 0;
int fildes[2];
char *argv1[10], *argv2[10];
char *cursor = "hello:";
int main();

void parseCommandLine()
{
    i = 0;
    strcpy(copy_command, command);
    token = strtok(command, " ");
    while (token != NULL)
    {
        argv1[i++] = token;
        token = strtok(NULL, " ");
        if (token && !strcmp(token, "|"))
        {
            piping = true;
            break;
        }
    }
    argv1[i] = NULL;
    argc = i;
}

/* This function is handeling the & command, check if a command ended with &*/
void check_amper()
{
    if (argc > 0 && !strcmp(argv1[argc - 1], "&"))
    {
        amper = true;
        argv1[argc - 1] = NULL;
    }
    else
        amper = false;
}

/* this function add the last commans to the history array in order to let us see the last command or go through them*/
void add_to_history()
{
    int index = history_index % 20;
    if (history[index] == NULL)
    {
        /* this doesnt work because its saves only the first word*/
        history[index] = strdup(copy_command);
    }
    else
    {
        free(history[index]);
        history[index] = strdup(copy_command);
    }
    history_index++;
}

/* This function is handeling the > command and the >> command*/
void redirection()
{
    if (argc > 1 && !strcmp(argv1[argc - 2], ">"))
    {
        redirect = true;
        outfile = argv1[argc - 1];
        argv1[argc - 2] = NULL;
        argc = argc - 2;
    }
    else if (argc > 1 && !strcmp(argv1[argc - 2], ">>"))
    {
        concat = true;
        outfile = argv1[argc - 1];
        argv1[argc - 2] = NULL;
        argc = argc - 2;
    }
    else
    {
        redirect = false;
        concat = false;
    }
}

/* This function is handeling the writing to the stderr file*/
void handel_stderr()
{
    if (argc > 1 && !strcmp(argv1[argc - 2], "2>"))
    {
        outerr = true;
        argv1[argc - 2] = NULL;
        outfile = argv1[argc - 1];
    }
    else
        outerr = false;
}

/* This function is handeling command to change the cursor word (the oppening word in every command)*/
int change_cursor()
{
    if (argc == 3 && !strcmp(argv1[0], "prompt") && !strcmp(argv1[1], "="))
    {
        cursor = malloc(strlen(argv1[2]) + 1);
        strcpy(cursor, argv1[2]);
        return 1;
    }
    return 0;
}

/* This function is handeling the echo command that print the arguments to the screen*/
int echo()
{
    if (argc > 1 && !strcmp(argv1[0], "echo"))
    {
        if (argc == 2 && !strcmp(argv1[1], "$?"))
        {
            printf("%d\n", status);
        }
        else
        {
            for (i = 1; i < argc; i++)
            {
                if (argv1[i][0] == '$')
                {
                    char *var = getenv(argv1[i] + 1);
                    if (var)
                    {
                        printf("%s ", var);
                    }
                    else
                    {
                        printf("%s ", argv1[i]);
                        printf("%s ", var);
                    }
                }
                else
                    printf("%s ", argv1[i]);
            }
            printf("\n");
        }
        return 1;
    }
    return 0;
}

/* This function is handeling the cd command that let you change the directory of the shell*/
int cd()
{
    if (argc > 1 && !strcmp(argv1[0], "cd"))
    {
        if (argc == 2)
        {
            if (chdir(argv1[1]) == -1)
            {
                printf("%s: No such file or directory\n", argv1[1]);
            }
        }
        else
        {
            printf("cd: too many arguments\n");
        }
        return 1;
    }
    return 0;
}

/* This function is handeling the quit command that exit from the shell*/
void quit()
{
    if (argc == 1 && !strcmp(argv1[0], "quit"))
    {
        exit(0);
    }
}

void stop_signal()
{
    return;
}

/* This function is handeling the if the user write Control-C*/
void sigintHandler()
{
    pid_t pgid = getpgrp();
    signal(SIGTSTP, stop_signal);
    killpg(pgid, SIGTSTP);
    printf("\nYou typed Control-C! \n");
}

/* This function is handeling the | (piping) command*/
void check_piping()
{
    if (piping)
    {
        i = 0;
        while (token != NULL)
        {
            token = strtok(NULL, " ");
            argv2[i] = token;
            i++;
        }
        argv2[i] = NULL;
    }
}

int variable()
{
    if (argc == 3 && !strcmp(argv1[1], "=") && argv1[0][0] == '$')
    {
        setenv(argv1[0] + 1, argv1[2], 1);
        return 1;
    }
    return 0;
}

int read_command()
{
    if (argc == 4 && !strcmp(argv1[0], "echo") && !strcmp(argv1[1], "Enter") && !strcmp(argv1[2], "a") && !strcmp(argv1[3], "string"))
    {
        char words[2][1024];
        fgets(command, 1024, stdin);
        sscanf(command, "%s %s", words[0], words[1]);
        if (!strcmp(words[0], "read"))
        {
            char *var = argv1[1];
            char *value = (char *)malloc(sizeof(char) * 1024);
            scanf("%s", value);
            setenv(var, value, 1);
            return 1;
        }
        return 0;
    }
    return 0;
}

int search_history()
{
    int current_index = (history_index - 1) % 20;
    int ans = 0;
    if ((argc == 1) && (argv1[0][2] == 'A' || argv1[0][2] == 'B'))
    {
        printf("%s", history[(current_index) % 20]);
        ans = 1;
        int c;
        while ((c = getchar()) != 'Q')
        {
            if (c == '\033')
            {
                printf("\033[1A"); // line up
                printf("\x1b[2K"); // delete line
                getchar();
                switch (getchar())
                {
                case 'A': /* UP arrow */
                    if (history[(current_index - 1) % 20] == NULL)
                    {
                        printf("%s", history[(current_index) % 20]);
                    }
                    else if (current_index % 20 == history_index % 20)
                    {
                        current_index = (current_index + 1) % 20;
                        printf("%s", history[(current_index) % 20]);
                    }
                    else
                    {
                        current_index = (current_index - 1) % 20;
                        printf("%s", history[current_index]);
                    }
                    break;

                case 'B': /* DOWN arrow*/
                    if ((current_index + 1) % 20 == history_index % 20)
                    {
                        printf("%s", history[(current_index) % 20]);
                    }
                    // else if (current_index % 20 == history_index)
                    // {
                    //     current_index = (current_index - 1) % 20;
                    //     printf("%s", history[(history_index) % 20]);
                    // }
                    else
                    {
                        current_index = (current_index + 1) % 20;
                        printf("%s", history[current_index]);
                    }
                    break;

                default:
                    break;
                }
            }
        }
    }
    return ans;
}

int main()
{
    int ans;
    history_index = 0;
    signal(SIGINT, sigintHandler);
    while (true)
    {
        printf("%s ", cursor);
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';
        piping = false;

        /* parse command line */
        parseCommandLine();

        /* Is command empty */
        if (argv1[0] == NULL)
            continue;

        /* Is command is !! check the last command and do it again */
        if (argc == 1 && !strcmp(argv1[0], "!!"))
        {
            if (history_index == 0)
            {
                printf("there is no history to the shell \n");
            }
            else
            {
                int index = (history_index - 1) % 20;
                memset(command, 0, sizeof(command));
                strcpy(command, history[index]);
                parseCommandLine();
            }
        }

        add_to_history();
        /* Does command contain pipe */
        check_piping();

        /* Does command line end with & */
        check_amper();

        /* Does command line end with > */
        redirection();

        /* Does command line end with 2> */
        handel_stderr();

        /* Does command line start with prompt */
        ans = change_cursor();
        if (ans == 1)
        {
            continue;
        }

        /* Does command line start with read */
        ans = read_command();
        if (ans == 1)
        {
            continue;
        }

        /* Does command line start with echo */
        ans = echo();
        if (ans == 1)
        {
            continue;
        }

        /* Does command line start with cd */
        ans = cd();
        if (ans == 1)
        {
            continue;
        }

        /* Does command line start with quit */
        quit();
        /* Does command line is $key = value */
        ans = variable();
        if (ans == 1)
        {
            continue;
        }

        ans = search_history();
        if (ans == 1)
        {
            continue;
        }

        /* for commands not part of the shell command language */
        if (fork() == 0)
        {
            /* redirection of IO ? */
            if (redirect)
            {
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
                /* stdout is now redirected */
            }
            if (concat)
            {
                fd = open(outfile, O_RDWR | O_CREAT | O_APPEND, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
                /* stdout is now redirected */
            }
            if (piping)
            {
                pipe(fildes);
                if (fork() == 0)
                {
                    /* first component of command line */
                    close(STDOUT_FILENO);
                    dup(fildes[1]);
                    close(fildes[1]);
                    close(fildes[0]);
                    /* stdout now goes to pipe */
                    /* child process does command */
                    execvp(argv1[0], argv1);
                }
                /* 2nd command component of command line */
                close(STDIN_FILENO);
                dup(fildes[0]);
                close(fildes[0]);
                close(fildes[1]);
                /* standard input now comes from pipe */
                execvp(argv2[0], argv2);
            }
            else if (outerr)
            {
                // redirect to stderr
                fd = creat(outfile, 0660);
                close(STDERR_FILENO);
                dup(fd);
                close(fd);
                /* stderr is now redirected */
                execvp(argv1[0], argv1);
            }
            else
                execvp(argv1[0], argv1);
        }
        /* parent continues over here... */
        /* waits for child to exit if required */
        if (amper == false)
            retid = wait(&status);
    }
}