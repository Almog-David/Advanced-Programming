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
#include <envz.h>

char command[1024];
char copy_command[1024];
char *history[20];
char *token;
char *outfile;
bool amper, redirect, retid, outerr, concat;
int i, status, argc, fd, first_index, last_index, numOfPipes;
int fildes[2];
char *argv1[10], ***pipe_command;
char *cursor = "hello:";
int main();

void execute();

/*This function is parsing the command and save it's words in argv1 + count number of pipes*/
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
            numOfPipes++;
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

/* This function add the last commans to the history array in order to let us see the last command or go through them*/
void add_to_history()
{
    int index = last_index % 20;
    if (history[index] == NULL) /* if it's one of the first 20 commands*/
    {
        history[index] = strdup(copy_command);
        last_index = (last_index + 1) % 20;
    }
    else /* we need to delete the old command and insert the new one*/
    {
        free(history[index]);
        history[index] = strdup(copy_command);
        last_index = (last_index + 1) % 20;
        first_index = (first_index + 1) % 20;
    }
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

/* This function is Redirecting the stderr output of the command to a file.*/
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

/* A function that run over the current signal SIGTSTP*/
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
    printf("%s ", cursor);
    fflush(stdout);
}

/* This function is handeling the | (piping) command*/
void piping()
{
    if (fork() == 0)
    {
        int prev_pipe = STDIN_FILENO;
        for (i = 0; i < numOfPipes; i++)
        {
            pipe(fildes);
            if (fork() == 0)
            {
                // Redirect previous pipe to stdin
                if (prev_pipe != STDIN_FILENO)
                {
                    dup2(prev_pipe, STDIN_FILENO);
                    close(prev_pipe);
                }

                // Redirect stdout to current pipe
                dup2(fildes[1], STDOUT_FILENO);
                close(fildes[1]);

                // Start command
                if (execvp(pipe_command[i][0], pipe_command[i]) == -1)
                {
                    fprintf(stderr, "Error executing command: %s\n", strerror(errno));
                    _exit(1);
                }
            }

            // Close read end of previous pipe (not needed in the parent)
            close(prev_pipe);

            // Close write end of current pipe (not needed in the parent)
            close(fildes[1]);

            // Save read end of current pipe to use in next iteration
            prev_pipe = fildes[0];
        }
        // Get stdin from last pipe
        if (prev_pipe != STDIN_FILENO)
        {
            dup2(prev_pipe, STDIN_FILENO);
            close(prev_pipe);
        }

        // Start last command
        if (execvp(pipe_command[numOfPipes][0], pipe_command[numOfPipes]) == -1)
        {
            fprintf(stderr, "Error executing command: %s\n", strerror(errno));
            exit(1);
        }
    }
    if (amper == false)
        retid = wait(&status);
}

/* This function check if we got pipe and split the command to sectors*/
void check_piping()
{
    if (numOfPipes > 0)
    {
        i = 0;
        int row = 0, colm = 0;
        int max_rows = numOfPipes + 1;
        int max_colm = 10;

        pipe_command = malloc(max_rows * sizeof(char **));
        for (int i = 0; i < max_rows; i++)
        {
            pipe_command[i] = malloc(max_colm * sizeof(char *));
        }

        while (argv1[i] != NULL)
        {
            if (!strcmp(argv1[i], "|"))
            {
                pipe_command[row][colm] = NULL;
                row++;
                colm = 0;
            }
            else
            {
                pipe_command[row][colm] = malloc((strlen(argv1[i]) + 1) * sizeof(char));
                strcpy(pipe_command[row][colm], argv1[i]);
                colm++;
            }
            i++;
        }
        pipe_command[row][colm] = NULL;
        piping();
        /* free the pipe_command memory */
        for (int i = 0; i < max_rows; i++)
        {
            for (int j = 0; j < max_colm; j++)
            {
                free(pipe_command[i][j]);
            }
            free(pipe_command[i]);
        }
        free(pipe_command);
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
        command[strlen(command) - 1] = 0;
        sscanf(command, "%s %s", words[0], words[1]);
        if (!strcmp(words[0], "read"))
        {
            char *var = words[1];
            char *value = (char *)malloc(sizeof(char) * 1024);
            fgets(value, 1024, stdin);
            value[strcspn(value, "\n")] = '\0';
            setenv(var, value, 1);
            return 1;
        }
        return 0;
    }
    return 0;
}
/* This function allow you to go through the command history */
int search_history()
{
    int current_index = (last_index - 1) % 20;
    int ans = 0;
    if ((argc == 1) && (history[current_index] != NULL) && (argv1[0][2] == 'A' || argv1[0][2] == 'B'))
    {
        printf("\033[1A"); // line up
        printf("\x1b[2K"); // delete line
        printf("%s", history[(current_index) % 20]);
        ans = 1;
        int c;
        while (c != 'Q')
        {
            c = getchar();

            if (c == '\033')
            {
                printf("\033[1A"); // line up
                printf("\x1b[2K"); // delete line
                getchar();         // remove [
                switch (getchar())
                {
                case 'A': /* UP arrow */
                    if (getchar() == '\n')
                    {
                        if (current_index % 20 == first_index % 20)
                        {
                            printf("%s", history[(current_index) % 20]);
                        }
                        else
                        {
                            current_index = (current_index - 1) % 20;
                            printf("%s", history[current_index]);
                        }
                    }
                    break;

                case 'B': /* DOWN arrow*/
                    if (getchar() == '\n')
                    {
                        if ((current_index + 1) % 20 == last_index % 20)
                        {
                            printf("%s", history[(current_index % 20)]);
                        }
                        else
                        {
                            current_index = (current_index + 1) % 20;
                            printf("%s", history[current_index]);
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            else if (c == '\n') /* if the user press Enter - repeat the command*/
            {
                memset(command, 0, sizeof(command));
                strcpy(command, history[current_index]);
                execute();
                current_index = (last_index - 1) % 20;
                printf("%s", history[current_index]);
            }
        }
    }
    return ans;
}

/* This function is handeling the if statement */
int if_statement()
{
    int ans = 0;
    char cut_command[1024];
    char direct[] = " > /dev/null";
    if (argc > 1 && !strcmp(argv1[0], "if"))
    {
        strcpy(cut_command, copy_command + 3); /* save the string without the if in cut_command*/
        strcat(cut_command, direct);           /* add to the cut command string the file direct*/
        fgets(command, 1024, stdin);           /* get a new input from the user - we need it to be "then"*/
        command[strlen(command) - 1] = 0;      /* remove the last char "\n"*/
        if (strcmp(command, "then") == 0)      /* if the next input from the user is then - activate*/
        {
            ans = 1;
            memset(command, 0, sizeof(command)); /* reset command*/
            strcpy(command, cut_command);        /*copy to command the cut command + direct*/
            execute();                           /* run the command in the execute function*/
            fgets(command, 1024, stdin);
            command[strlen(command) - 1] = 0;
            if (status == 0)
            { /* if status is equal to zero - do everything until fi or else*/
                while ((strcmp(command, "else") != 0) && (strcmp(command, "fi") != 0))
                {
                    execute();
                    fgets(command, 1024, stdin);
                    command[strlen(command) - 1] = 0;
                }
                if (strcmp(command, "else") == 0)
                {
                    while (strcmp(command, "fi") != 0)
                    {
                        fgets(command, 1024, stdin);
                        command[strlen(command) - 1] = 0;
                    }
                }
            }
            else
            {
                /* if status is not equal to zero - skip until else and then do everything until fi*/
                while ((strcmp(command, "else") != 0) && (strcmp(command, "fi") != 0))
                {
                    fgets(command, 1024, stdin);
                    command[strlen(command) - 1] = 0;
                }
                if (strcmp(command, "else") == 0)
                {
                    while (strcmp(command, "fi") != 0)
                    {
                        execute();
                        fgets(command, 1024, stdin);
                        command[strlen(command) - 1] = 0;
                    }
                }
            }
        }
    }
    return ans;
}
/* Check and execute all the command's types we have*/
void execute()
{
    /* parse command line */
    parseCommandLine();

    /* Is command empty */
    if (argv1[0] == NULL)
        return;

    /* Is command is !! check the last command and do it again */
    if (argc == 1 && !strcmp(argv1[0], "!!"))
    {
        if (history[first_index] == NULL)
        {
            printf("there is no history to the shell \n");
        }
        else
        {
            int index = (last_index - 1) % 20;
            memset(command, 0, sizeof(command));
            strcpy(command, history[index]);
            parseCommandLine();
        }
    }

    if (search_history())
    {
        return;
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
    if (change_cursor())
    {
        return;
    }

    /* Does command line start with read */
    if (read_command())
    {
        return;
    }

    /* Does command line start with echo */
    if (echo())
    {
        return;
    }

    /* Does command line start with cd */
    if (cd())
    {
        return;
    }

    /* Does command line start with quit */
    quit();
    /* Does command line is $key = value */
    if (variable())
    {
        return;
    }
    /* Does command line start with if statement */
    if (if_statement())
    {
        return;
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
        if (outerr)
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
    return;
}

int main()
{
    first_index = 0, last_index = 0;
    signal(SIGINT, sigintHandler);
    while (true)
    {
        printf("%s ", cursor);
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';
        numOfPipes = 0;
        execute();
    }
}
