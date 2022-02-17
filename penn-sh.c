#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tokenizer.h"
#include <stdlib.h>
#include <fcntl.h>

#define INPUT_SIZE 1024

pid_t childPid = 0;

void executeShell(int timeout);

void writeToStdout(char *text);

//void alarmHandler(int sig);

void sigintHandler(int sig);

char **getCommandFromInput();

void registerSignalHandlers();

void killChildProcess();

void redirectionsSTDOUTtoFile();

void redirectionsSTDINtoFile();

int main(int argc, char **argv) {
    registerSignalHandlers();

    int timeout = 0;
    if (argc == 2) {
        timeout = atoi(argv[1]);
    }

    if (timeout < 0) {
        writeToStdout("Invalid input detected. Ignoring timeout value.\n");
        timeout = 0;
    }

    while (1) {
        executeShell(timeout);
    }

    return 0;
}

/* Sends SIGKILL signal to a child process.
 * Error checks for kill system call failure and exits program if
 * there is an error */
void killChildProcess() {
    if (kill(childPid, SIGKILL) == -1) {
        perror("Error in kill");
        exit(EXIT_FAILURE);
    }
}

/* Signal handler for SIGALRM. Catches SIGALRM signal and
 * kills the child process if it exists and is still executing.
 * It then prints out penn-shredder's catchphr
 * ase to standard output */
//void alarmHandler(int sig) {
//}

/* Signal handler for SIGINT. Catches SIGINT signal (e.g. Ctrl + C) and
 * kills the child process if it exists and is executing. Does not
 * do anything to the parent process and its execution */
void sigintHandler(int sig) {
    if (childPid != 0) {
        killChildProcess();
    }
}


/* Registers SIGALRM and SIGINT handlers with corresponding functions.
 * Error checks for signal system call failure and exits program if
 * there is an error */
void registerSignalHandlers() {
    if (signal(SIGINT, sigintHandler) == SIG_ERR) {
        perror("Error in signal");
        exit(EXIT_FAILURE);
    }
}

/* Prints the shell prompt and waits for input from user.
 * Takes timeout as an argument and starts an alarm of that timeout period
 * if there is a valid command. It then creates a child process which
 * executes the command with its arguments.
 *
 * The parent process waits for the child. On unsuccessful completion,
 * it exits the shell. */
void executeShell(int timeout) {
    // printf("enter executeShell\n");
    char **commandArray;
    int status;  //for parent to keep track when the child is terminated
    char minishell[] = "penn-sh> ";
    writeToStdout(minishell);

    commandArray = getCommandFromInput();
    if(commandArray == NULL){
        free(commandArray);
        return;
    }
    //check crt D
    // if(commandArray[0][0] == '\0'){
    //     exit(EXIT_FAILURE);
    // }


    if(commandArray[0] != NULL){
        childPid = fork();
        if(childPid < 0){
            perror("Error in creating child process");
            exit(EXIT_FAILURE);
        }
        
        if(childPid == 0){
            //int saved_stdout = dup(STDOUT_FILENO);
            int m = 0;
            char **args = calloc(INPUT_SIZE, sizeof(char *));
            //int countIN = 0, countOUT = 0;

            int in_mode = 0, out_mode = 0;
            char* in_file = NULL;
            char* out_file = NULL;

            while(commandArray[m] != NULL){
                if(in_mode == 1){
                    if(in_file != NULL){
                        perror("Invalid: Multiple Standard Iutput Redirects. \n");
                        exit(EXIT_FAILURE);
                    }
                    in_file = commandArray[m];
                    in_mode = 0;
                    m++;
                }
                else if(out_mode == 1){
                    if(out_file != NULL){
                        perror("Invalid: Multiple Standard Output Redirects. \n");
                        exit(EXIT_FAILURE);
                    }
                    out_file = commandArray[m];
                    out_mode = 0;
                    m++;
                }
                else{
                    if(commandArray[m][0] == '<'){
                        in_mode = 1;
                        m++;
                    }
                    else if(commandArray[m][0] =='>'){
                        out_mode = 1;
                        m++;
                    }else{
                        args[m] = commandArray[m];
                        m++;
;                    }
                }
            }

           
            args[m] = NULL;
            redirectionsSTDOUTtoFile(out_file);
            redirectionsSTDINtoFile(in_file);    

            int error_code = execvp(*args, args);
            if (error_code == -1) {
                perror("Error in execvp");
                exit(EXIT_FAILURE);
            }
            free(args);
  
        }

        //parent process
        else{
            do {
                if (wait(&status) == -1) {
                    perror("Error in child process termination");
                    exit(EXIT_FAILURE);
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            //reset the childePid
            childPid = 0;
        }
    }
    free(commandArray);
}

//Redirect out
void redirectionsSTDOUTtoFile(char *tokenAfter){
    int new_stdout = open(tokenAfter, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    dup2(new_stdout, STDOUT_FILENO);
    close(new_stdout);
}

//Redirect in
void redirectionsSTDINtoFile(char *tokenAfter){  
    //check the file exist  
    if(tokenAfter != NULL){
        int new_stdin = open(tokenAfter, O_RDONLY);
        if(new_stdin == -1){
            perror("Invalid input");
            exit(EXIT_FAILURE);
        }
        dup2(new_stdin, STDIN_FILENO);
        close(new_stdin);
    }
    
}

/* Writes particular text to standard output */
void writeToStdout(char *text) {
    if (write(STDOUT_FILENO, text, strlen(text)) == -1) {
        perror("Error in write");
        exit(EXIT_FAILURE);
    }
}

/* Reads input from standard input till it reaches a new line character.
 * Checks if EOF (Ctrl + D) is being read and exits penn-shredder if that is the case
 * Otherwise, it checks for a valid input and adds the characters to an input buffer.
 *
 * From this input buffer, the first 1023 characters (if more than 1023) or the whole
 * buffer are assigned to command and returned. An \0 is appended to the command so
 * that it is null terminated */
char **getCommandFromInput() {
   
  TOKENIZER *tokenizer;
  char *tok = NULL;
  char **command_array = calloc(INPUT_SIZE, sizeof(char *));
  char *input_buffer = (char *) malloc(INPUT_SIZE*sizeof(char));

  int num_bytes = read(STDIN_FILENO, input_buffer, INPUT_SIZE);
  if (num_bytes == 1) {
      free(command_array);
      return NULL;
  }

  int length_string = 0;
  
  int idx = 0;
  tokenizer = init_tokenizer(input_buffer);
  while ((tok = get_next_token(tokenizer)) != NULL) {
      length_string = strlen(tok);
      command_array[idx] = calloc(length_string + 1, sizeof(char));
      for (int i = 0; tok[i] != '\0'; i++) {
          command_array[idx][i] = tok[i];
      }
      command_array[idx][length_string] = '\0';
      idx++;
      free(tok);
  }
  command_array[idx] = NULL;
  free_tokenizer(tokenizer);
  return command_array;
}
