/* UNIXShell.c is a c program to create a shell interface into a UNIX system
 * that is meant to accept user commands and execute on child processes
 * This code accepts user input, parses it into commands, and then creates
 * a child process to run the command given, meaning only one command can be
 * inputted at a time. If the user uses a '&' to indicate a background process,
 * The program will add it to a linked list to keep track of all the currently
 * active processes.
 * 
 * @author Sam Schmitz (sam98)
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include<fcntl.h>
#include <sys/wait.h>

#define MAX_LINE 80  /* The maximum length command */
struct Node* head;   /* The head node for the linked list of background processes */
int bp_size = 0;     /* The amount of background processes in this session (size of list) */

//The node data structure for linked lists
struct Node  {
	int data; //process id
    int bp_num; //counter of which process this is (starts at 1)
    int command_size; //size of array containing commands for this process
    char **command; //array containing commands for this process
	struct Node* next;
};

//Returns if the linked list is empty
int isEmpty(){
    if (head == NULL){
        return 1;
    }
    return 0;
}

//Creates a new Node with the given parameters
struct Node* GetNewNode(int x, char **command, int command_size) {
    bp_size++;
	struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
	newNode->data = x;
    newNode->bp_num = bp_size;
    newNode->command = malloc(sizeof(char*)*command_size);
    for (int i = 0; i < command_size; i++){                 /*This for loop */
        newNode->command[i] = malloc(sizeof(command[i]));   /*Is to create a new */
        strcpy(newNode->command[i], command[i]);            /*copy of the string array */
    }
    newNode->command_size = command_size;
	newNode->next = NULL;
	return newNode;
}

//Adds a new node at the start of the list
int Insert(int x, char **command, int command_size) {
	struct Node* newNode = GetNewNode(x, command, command_size);
	if(head == NULL) {
		head = newNode;
		return bp_size;
	}
	newNode->next = head; 
	head = newNode;
    return bp_size;
}

//Removes first Node found with given process id
int Remove(int x){
    struct Node* temp = head; //Temp Node for navigation
    struct Node* trash = temp; //Node to free memory
    if (head->data == x){
        if (head->next == NULL){
            head = NULL;
            free(trash);
            return 1;
        }else{
            head = head->next;
            free(trash);
            return 1;
        }
    }
    while (temp->next != NULL){
        if (temp->next->data == x){
            if (temp->next->next == NULL){
                trash = temp->next;
                temp->next = NULL;
                free(trash);
                return 1;
            }
            trash = temp->next;
            temp->next = temp->next->next;
            free(trash);
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}

//Iterates through the list of processes and prints status of those done and removes them
void printFinished(){
    if (!isEmpty()){
        struct Node *temp = head;
        int status;
        while (temp->next != NULL){  //This while loop iterates through all but last node
            int wpid = waitpid(temp->data, &status, WNOHANG);
            if (WIFSIGNALED(status)){ //If process exited by command
                printf("[%d] Terminated", temp->bp_num);
                for (int j = 0; j < temp->command_size; j++){ //Print original command of process
                    printf(" %s", temp->command[j]);
                }
                printf("\n");
                int num = temp->data;
                temp = temp->next;
                Remove(num);
                return;
            }   
            else if (wpid == -1){ //Other finished processes
                if(WEXITSTATUS(status) == 0){ //Zero exit code
                    printf("[%d] Done", temp->bp_num);
                } else{ //Nonzero exit code
                    printf("[%d] Exit %d", temp->bp_num, WEXITSTATUS(status));
                }
                for (int j = 0; j < temp->command_size; j++){ //Print original command
                    printf(" %s", temp->command[j]);
                }
                printf("\n");
                int num = temp->data;
                temp = temp->next;
                Remove(num);
            } else{ //Next process
                temp = temp->next;
            }
        }
        int wpid = waitpid(temp->data, &status, WNOHANG); //Last process in list
        if (WIFSIGNALED(status)){ //If process exited by command
            printf("[%d] Terminated", temp->bp_num);
            for (int j = 0; j < temp->command_size; j++){ //Print original command of process
                printf(" %s", temp->command[j]);
            }
            printf("\n");
            int num = temp->data;
            temp = temp->next;
            Remove(num);
            return;
        }   
        else if (wpid == -1){ //Other finished processes
            if(WEXITSTATUS(status) == 0){ //Zero exit code
                printf("[%d] Done", temp->bp_num);
            } else{ //Nonzero exit code
                printf("[%d] Exit %d", temp->bp_num, WEXITSTATUS(status));
            }
            for (int j = 0; j < temp->command_size; j++){ //Print original command
                printf(" %s", temp->command[j]);
            }
            printf("\n");
            int num = temp->data;
            Remove(num);
            
        }
    }

}

int main(void) {
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    char *redirection_args[4] = {'\0', '\0', '\0', '\0'}; /* array for printing input redirection */
    int should_run= 1;      /* flag to determine when to exit program */
    char string[MAX_LINE];  /*string inputted by user */
    int i;                  /*iterator; acts as count of command arguments*/
    int out_flag = 0;       /* flag if output redirection indicated */
    int in_flag = 0;        /* flag is input redirection indicated */
    int background_flag = 0;/* flag if background process indicated */
    int kill_flag = 0;      /* flag if termination indicated */
    whilestart:;            /*go to marker for kill command */
    while (should_run) {
        int out_file_desc = STDOUT_FILENO; /*resetting Standard Out */
        int in_file_desc = STDIN_FILENO;   /*resetting Standard In */
	    for (int j = 0; j <= MAX_LINE/2; j++){ /*resetting arguments array */
	        args[j] = '\0';
	    }
        printFinished(); 
        printf("352> ");
        fflush(stdout);
        fgets(string, MAX_LINE, stdin);
        if (!strcmp("\n", string)){	 /*if no input when enter is pressed */
            printFinished();
            continue;
        }
	    i = 0;                                      /*resets iterator*/
	    char *token = strtok(string, " '\n'");
	    while(token != NULL){                       /* This loop acts as parser for user input */
            if (!strcmp(token, ">")){               /* If user indicates output */
                out_flag = 1;
                if (redirection_args[0] != '\0'){   /*Saves output argument string separatly */
                    redirection_args[2] = token;
                } else{
                    redirection_args[0] = token;
                }
            } else if (out_flag){                  /* Next iteration since output indicated */
                if (access(token, F_OK)==-1){
                    fopen(token, "w");
                }
                out_file_desc = open(token, O_WRONLY);
                out_flag = 0;
                if (redirection_args[1] != '\0'){
                    redirection_args[3] = token;
                } else{
                    redirection_args[1] = token;
                }
            } else if (!strcmp(token, "<")){        /* If user indicates input */
                in_flag = 1;
                if (redirection_args[0] != '\0'){   /*Saves input argument string separatly */
                    redirection_args[2] = token;
                } else{
                    redirection_args[0] = token;
                }
            } else if (in_flag){                    /* Next iteration since input indicated */
                if (access(token, F_OK)==-1){
                    fopen(token, "w");
                }
                in_file_desc = open(token, O_RDONLY);
                in_flag = 0;
                if (redirection_args[1] != '\0'){
                    redirection_args[3] = token;
                } else{
                    redirection_args[1] = token;
                }
            }else if (!strcmp(token, "kill")){   /*if user indicates termination */
                kill_flag = 1;
            }else if (kill_flag){
                kill_flag = 0;
                kill(atoi(token), SIGTERM);
                sleep(1);
                goto whilestart; //Break nested while loops
            }else if (!strcmp(token, "&")){      /*If user indicates background process */
                background_flag = 1;
            }else{                               /*Else, add argument to array*/
	            args[i] = token;
                i++;
            }
	        token = strtok(NULL, " \n");         //continues parsing
	    }
        if (args[0] != NULL){
            if(!strcmp("exit", args[0])){        /*If user indicates exit*/
                should_run = 0;
            }else{
	            pid_t pid;   //Process ID
	            int status;  //Status      
	            pid = fork();
	            if(pid == 0) {        /*If child process*/
                    dup2(out_file_desc, STDOUT_FILENO);  /*Changes standard out*/
                    dup2(in_file_desc, STDIN_FILENO);    /*Changes standard in*/
                    execvp(args[0], args);
                    printf("Unknown command\n");
                    exit(0);
	            } else {           /*If parent process*/
                    pid_t tpid;
                    if (!background_flag){  //Not a background process
                        do {
	                        tpid = wait(&status);
                        } while(tpid != pid);
                    }else {                //Is a background process
                        background_flag = 0;
                        for (int j = 0; j < 4; j++){  //This loop adds redirection commands to argument string for printing purposes
                            if (redirection_args[j] != '\0'){
                                args[i] = redirection_args[j];
                                i++;
                            } else {
                                break;
                            };
                        }
                        int id = Insert(pid, args, i); //Adds background process to Linked list
                        for (int j = 0; j < 4; j++){  //Resets redirection array
                            redirection_args[j] = '\0';
                        }
                        printf("[%d] %d\n", id, pid);
                    } 
                }
            }
        }
        dup2(STDOUT_FILENO, STDOUT_FILENO);
    }   
    return 0;
}