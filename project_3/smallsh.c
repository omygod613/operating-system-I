#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*********************************************************************
 *         childrend processes
 *********************************************************************/
struct child_list {
        unsigned int num;
        unsigned int cap;
        pid_t *children;
} bg_child_list;

/*********************************************************************
 *         record precess status
 *********************************************************************/
int shell_status = 0;
char *echo_string[100];
char echo_buf[100];
int i = 0;

/*********************************************************************
 *         declare special functuions
 *********************************************************************/
void runloop();
char* create_file_token(char **word, unsigned int max_length);
void parse_and_run(char *line, unsigned int length);
bool word_has_comment(char *word);

/*********************************************************************
 *         declare function for signal
 *********************************************************************/
void trap_interrupt(int signum);
void print_args(const char **arr);
void catchint(int signo);
void child_ended(int signum);

void init_child_list();
void destroy_child_list();
void push_child_list(pid_t child);
pid_t pop_child_list();

/*********************************************************************
 *         intialize backgrounded children processes
 *********************************************************************/
void init_child_list() {
        bg_child_list.num = 0;
        bg_child_list.cap = 4;
        bg_child_list.children = malloc(bg_child_list.cap * sizeof(pid_t));
}

/*********************************************************************
 *         kill all children for exit the program
 *********************************************************************/
void destroy_child_list() {
        free(bg_child_list.children);
}

/*********************************************************************
 *         puch the child process to the children list, 
 *         based on the pid of the child process
 *********************************************************************/
void push_child_list(pid_t child) {
        if (bg_child_list.num == bg_child_list.cap) {
                bg_child_list.cap *= 2;
                bg_child_list.children = realloc(bg_child_list.children,
                                bg_child_list.cap * sizeof(pid_t));
        }
        bg_child_list.children[bg_child_list.num] = child;
        bg_child_list.num++;
}

/*********************************************************************
 *         pop the child process out of the children list
 *********************************************************************/
pid_t pop_child_list() {
        if (bg_child_list.num > 0) {
                bg_child_list.num--;
                return bg_child_list.children[bg_child_list.num + 1];
        } else {
                return 0;
        }
}

/*********************************************************************
 *         main function
 *********************************************************************/
int main() {
        init_child_list();
        // Trap SIGINT. This is a shell, so kill children, not current process.
        signal(SIGINT, trap_interrupt);
        signal(SIGTSTP, child_ended);
        runloop();
        return 0;
}

/*********************************************************************
 *         interrupt the program and kill the children process
 *********************************************************************/
void trap_interrupt(int signal) {
   pid_t pid = pop_child_list();
    
    if (pid != 0) {
        // Sends SIGINT to every process in the same process group with pid
        kill(pid, SIGKILL); // signals to the entire foreground process group
  }
  if (WTERMSIG(shell_status))
                        printf("Termination signal: %d\n",
                                        WTERMSIG(signal)); 
    return; 
}

/*********************************************************************
 *         check the children process is background or not
 *         based on the pid of the child process
 *********************************************************************/
bool was_bgd(pid_t pid) {
        int i = 0;
        for (i = 0; i < bg_child_list.num; i++) {
                if (pid == bg_child_list.children[i]) {
                        return true;
                }
        }
        return false;
}

/*********************************************************************
 *         print the exit status and pid, 
 *         if the background child exits
 *********************************************************************/
void child_ended(int signum) {
        int child_info;
        pid_t pid = waitpid(0, &child_info, 0);
        if (was_bgd(pid)) {
                printf("%d\n%d\n", pid, WEXITSTATUS(child_info));
        }
    return;
}

/*********************************************************************
 *         read a command, parse it, execute it
 *********************************************************************/
void runloop() {
        char *line = NULL;
        size_t linecap = 0;
        ssize_t line_len;
        printf(":");
        //get line from user's command
        while ((line_len = getline(&line, &linecap, stdin)) > 0) {
                parse_and_run(line, (unsigned int)line_len);
                printf(":");
                fflush(stdout);
        }
        destroy_child_list();
}

/*********************************************************************
 *         parse a given line and execute it
 *********************************************************************/
void parse_and_run(char *line, unsigned int length) {
        // command [arg1 arg2 ...] [< input_file] [> output_file] [&]


        // it can guarantee that the arguments are less than the characters
        // read from the user.
        char **args = malloc(length * sizeof(char*));

        // separate the arguments to a command
        const char *sep = " \n";
        const char devnull[] = "/dev/null";

        int infile = STDIN_FILENO;
        int outfile = STDOUT_FILENO;
        pid_t pid;

        char *word = NULL;
        char *input = NULL;
        char *output = NULL;
        bool is_background = false;

        unsigned int args_index = 1;

        // each command applied for the instruction
        char *command = strtok(line, sep);
        if (command == NULL) goto cleanup;
        args[0] = command;
        // built in exit command. cleanup and exit 0.
        if (strcmp(command, "exit") == 0) {
                trap_interrupt(-1);
                destroy_child_list();
                free(args);
                free(line);
                exit(0);
        } else if (strcmp(command, "cd") == 0) {
                // Built in cd command.
                // Get next token
                char *dest = strtok(NULL, sep);
                // if the next token is NULL, set the destination to $HOME
                if (dest == NULL) {
                        dest = getenv("HOME");
                }
                // cd to destination.
                if(chdir(dest) == -1) {
                        perror("chdir");
                        goto cleanup;
                }
                goto cleanup;
        } else if (strcmp(command, "status") == 0) {
                // Built in status command. Print status information.
                // Print status information.
                // If the child exited, print its status
                if (WIFEXITED(shell_status))
                        printf("exit value %d\n", WEXITSTATUS(shell_status));
                // If it was signalled, print the signal it received.
                if (WIFSIGNALED(shell_status))
                        printf("stop signal %d\n", WSTOPSIG(shell_status));
                // If it was terminated, print the termination signal.
                if (WTERMSIG(shell_status))
                        printf("terminated by signal %d\n",
                                        WTERMSIG(shell_status));
                goto cleanup;
        } 

        else if (strncmp(command, "#", 1) == 0) {
                        goto cleanup;}
     
        // For each argument in the command.
        do {
                word = strtok(NULL, sep);
                // if the word is NULL, we're done.
                if (word == NULL) {
                        break;
                } else if (word_has_comment(word)) {
                        // if the word has a comment, nuke the comment
                        // and add the commentless word to the arguments.
                        args[args_index] = word;
                        args_index++;
                        break;
                } 
                else if (strncmp(word, "<", 1) == 0) {
                        // If the infile is specified, get the input file name.
                        word = strtok(NULL, sep);
                        if (word == NULL) {
                                printf("invalid syntax. "
                                       "no input file provided\n");
                                goto cleanup;
                        }
                        input = create_file_token(&word, length);
                        continue;
                } else if (strncmp(word, ">", 1) == 0) {
                        // If the outfile is specified, get the output file
                        // name.
                        word = strtok(NULL, sep);
                        if (word == NULL) {
                                printf("invalid syntax. "
                                       "no output file provided\n");
                                goto cleanup;
                        }
                        output = create_file_token(&word, length);
                        continue;
                } else if (strncmp(word, "&", 1) == 0) {
                        // Background the process.
                        is_background = true;
                        break;
                } else if (strncmp(word, "$$", 2) == 0){
                        trap_interrupt;
                        goto cleanup;
                }
                // Push the word onto the list of arguments.
                args[args_index] = word;
                args_index++;
        } while (word != NULL);
        // add the trailing null pointer.
        args[args_index] = NULL;

        if (is_background && input == NULL) {
                // redirect to /dev/null
                input = (char*)devnull;
        }
        if (is_background && output == NULL) {
                // redirect to /dev/null
                output = (char*)devnull;
        }

        // Fork. The program splits into two branches a child and a parent.
        // The parent has pid set to the child's pid, and the child has
        // pid set to 0.

        // split the program into a child and a praent
        // the parent's pid > 0, the child's pid is 0.
        pid = fork();
        if (pid == 0) { // child
                // If the input file has been specified, set child's stdin
                // to input.
                if (input != NULL) {
                        // open the input file
                        infile = open(input, O_RDONLY);
                        // If opening the input file fails, print reason.
                        if (infile == -1) {
                                perror("infile");
                                free(args);
                                free(line);
                                destroy_child_list();
                                // exit 1 according to specs.
                                exit(1);

                        } else {
                                // redirect stdin to the input file descriptor.
                                dup2(infile, STDIN_FILENO);
                                // close the file.
                                close(infile);
                        }
                }
                if (output != NULL) {
                        // open the output file, 
                        // creating it if it doesn't exist
                        outfile = open(output, O_WRONLY | O_CREAT, 0744);
                        // If opening the output file fails, print reason.
                        if (outfile == -1) {
                                perror("outfile");
                                free(args);
                                free(line);
                                destroy_child_list();
                                // Exit 1 according to specs.
                                exit(1);

                        }
                        if (output != devnull) {
                                // Same as $ chmod 0644 output
                                if (chmod(output, S_IRUSR | S_IWUSR | S_IRGRP |
                                                        S_IROTH) == -1) {
                                        perror("chmod");
                                }
                        }
                        // redirect the output file descriptor to stdout
                        dup2(outfile, STDOUT_FILENO);
                        close(outfile);
                }

                // execute the command
                execvp(command, args);
                // if execution fails, 
                // print the error, clean up allocated memory, and exit.
                perror("execvp");
                free(args);
                free(line);
                destroy_child_list();
                // exit 1 according to specs.
                exit(1);

        } else { // parent
                // If the child process is not backgrounded, wait for it.
                if (!is_background) {
                        waitpid(pid, &shell_status, 0);
                } else {
                        // otherwise, add it to the list of backgrounded
                        // processes to be signaled at exit.
                        push_child_list(pid);
                        printf("background pid is %d\n", pid);
                        
                        // if it was terminated, print the termination signal.
                        if (!WTERMSIG(shell_status))
                                printf("exit value: %d\n", WEXITSTATUS(shell_status));

                }
        }
    cleanup:
        // If the input is not the statically allocated
        // /dev/null, free it
        if (input != devnull) {
                free(input);
        }
        // If the output is not the statically allocated
        // /dev/null, free it
        if (output != devnull) {
                free(output);
        }
        // free the list of arguments.
        free(args);
}

/*********************************************************************
 *         print the arguments for debugging
 *********************************************************************/
void print_args(const char **arr) {
        int i = 0;
        for (i = 0; arr[i] != NULL; i++) {
                printf("%s, ", arr[i]);
        }
        printf("\n");

}

/*********************************************************************
 *         determine whether the word hashtag
 *********************************************************************/
bool word_has_comment(char *word) {
        int i = 0;
        for (i = 0; word[i] != '\0'; i++) {
                if (word[i] == '#') {
                        word[i] = '\0';
                        return true;
                }
        }
        return false;
}

/*********************************************************************
 *         create a file
 *********************************************************************/
char* create_file_token(char **word, unsigned int max_length) {
        size_t token_length = strnlen(*word, max_length);
        char *token = malloc(token_length * sizeof(char));
        strncpy(token, *word, token_length);
        return token;
}