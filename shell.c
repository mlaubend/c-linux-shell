#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>

#define MAX_LINE 80

void cd(char*[MAX_LINE]);
char* trimwhitespace(char*);

int main(void)
{
    char *args[MAX_LINE/2 + 1];
    int should_run = 1;

    while (should_run)
    {
        int noCommands = 0;
        char command[MAX_LINE];
        int redirect = 0;

        pid_t id;
        int status = 0;

        printf("osh>");
        fflush(stdout);

        //fgets stores the entire input as one string called command
        fgets(command, MAX_LINE, stdin);

        const char look[2] = " ";
        char* redirectors[9] = {">", "1>", "2>", ">>", "2>>", "&>", "<", "|", "&"};
        char *next_command;

        //strtok looks for the first space and splits up the string at the first whitespace
        next_command = strtok(command, look);

        //this loop iterates over the entire command string and splits up the string into it's remaining pieces based on the whitespace
        while (next_command != NULL)
        {
            //saving each piece of the larger string into the args array
            args[noCommands] = trimwhitespace(next_command);
            noCommands++;

            //looking for the next substring based on the last whitespace and the next whitespace
            next_command = strtok(NULL, look);
        }

        //search the list for any redirect commands.  If any are found, a specific integer is saved in int redirect
        int b = 0;
        for (b; b < 9; b++)
        {
            if (strcmp(args[1], redirectors[b]) == 0)
            {
                redirect = b + 1;
                break;
            }
        }

        //cd command
        int changeDir = strcmp(args[0], "cd");
        int exit = strcmp(args[0], "exit");
        if(changeDir == 0)
        {
            cd(args);
            printf("%s\n", get_current_dir_name());
        }

        //if the user want's to exit
        else if(exit == 0)
        {
            should_run = 0;
        }

        //else it's a command
        else
        {
            char BUFFER[MAX_LINE];
            char* arguments[noCommands];
            char DIRECTORY[MAX_LINE];
            char* bin = "/bin/";

            //this is another array that will be deleted each time the loop runs so it won't overflow
        int a = 0;
        for (a; a < noCommands; a++)
        {
            arguments[a] = args[a];
        }

        arguments[noCommands] = NULL;
        args[MAX_LINE/2 + 1] = NULL;
        strcpy(DIRECTORY, bin);
        strcat(DIRECTORY, arguments[0]);

        //& will envoke a child without envoking wait()
        if (redirect == 9)
        {
            id = fork();
            if(!id)
            {
                execv(DIRECTORY, arguments);
            }
        }

        //forking the main process and envoking wait
        id = fork();
        wait(&status);

        //if the SIGINT signal is sent, it will kill the process id
        kill(SIGINT, id);

        if(!id)
        {
            //create and open the file I am writing to
            int file = open(arguments[2], O_CREAT | O_WRONLY);

            switch (redirect)
            {
                //no redirect
                case 0:
                {
                    /***
                    1: RASPI PATH IS: '/USR/BIN/, UBUNTU PATH IS: '/BIN/'
                    ***/
                    execv(DIRECTORY, arguments);
                    break;
                }

                //the stdout redirect to a file
                case 1:
                {
                    //change the stdout of the file
                    dup2(file, 1);
                }

                //same as case 1
                case 2:
                {
                    //change the stdout of a command to a file
                    dup2(file, 1);
                    break;
                }

                //redirect stderr of a command to a file
                case 3:
                {
                    //change the stderr of the file
                    dup2(file, 2);
                    break;
                }
            }

            //this appends the stdout to an already existing file
            if (redirect == 4)
            {
                FILE* fd;
                char filepath[MAX_LINE];
                getwd(filepath);
                strcat(filepath, "/");
                strcat(filepath, arguments[2]);
                printf("\n%s", filepath);

                fd = fopen(filepath, "a");
                dup2(file, 1);
                execlp(DIRECTORY, getwd(BUFFER));
                fclose(fd);
            }

            //write the output of the command to the file
            execlp(DIRECTORY, getwd(BUFFER));
            close(file);
        }
    }
}
  return 0;
}

//this function trims the rest of the whitespace from the arguments
char* trimwhitespace(char* str)
{
    char *end;

    // Trim leading space
    while(isspace(*str)) str++;

    //this returns the string if it consists of only spaces
    if(*str == 0)
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace(*end)) end--;

    // Write new null terminator
    *(end+1) = 0;

    return str;
}

void cd(char* inputpath[MAX_LINE])
{
    //const char slash[2] = "/";
    char buildDir[MAX_LINE]; //this will be the directory path that is sent to chdir() - but first we have to build it


    //if the input contains no path, bring back to the home directory
    if (strstr(inputpath[1], " ") == NULL)
    {
        chdir("/home/mlaubend");
    }

    //if the inputpath does not contain a slash.  This won't work for multiple directory changes which contain slashes. fuck.
    if (strstr(inputpath[1], "/") == NULL)
    {
        //retrieves the present working directory
        getcwd(buildDir,sizeof(buildDir));

        //concatenates buildDir with a /
        strcat(buildDir,"/");

        //concatenates buildDir with the inputpath
        strcat(buildDir, inputpath[1]);

        //system call to change directory
        chdir(buildDir);
    }

    else
    {
        //if the entire path was entered as an argument
        chdir(trimwhitespace(inputpath[1]));
    }
}
