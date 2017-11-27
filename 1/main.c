/*
*****************************************************************************************
*** ID                  : xxxxxxxxx
*** Name                : Victoria Sahle
*** Date                : January 10, 2016
*** Program Name        : main.c
*** Program Description : Assignment 1: A simple shell program similar to the Unix shell.
*****************************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>       
#include <readline/readline.h>
#include <readline/history.h>

#define true  1
#define false 0
/*Global variable to keep track of job number*/
int g_job_id;
pid_t bg_id;

/*Structure for nodes in my linked list*/
struct node
{   
    int job_count;
    char* status;
    char* path;
    pid_t background_id;
    struct node *next;
};

struct node *head = NULL;
struct node *curr = NULL;

/* Create a linked list for background processes  */
struct node* create_list(char* status, char* path, pid_t background_id)
{
    struct node *ptr = (struct node*)malloc(sizeof(struct node));
    if(NULL == ptr)
    {
        printf("\n Node creation failed \n");
        return NULL;
    }
    ptr->status = (char*) malloc(strlen(status) * sizeof(char));
    strcpy(ptr->status, status);
    ptr->path = (char*) malloc(strlen(path) * sizeof(char));
    strcpy(ptr->path, path);
    ptr->job_count = g_job_id;
    ptr->background_id = background_id;
    ptr->next = NULL;

    head = curr = ptr;
    return ptr;
}

/** Add a node (background process) to the linked list*/
struct node* add_to_list(char* status, char* path, pid_t background_id)
{
    if(NULL == head)
    {
        return (create_list(status, path, background_id));
    }

    struct node *ptr = (struct node*)malloc(sizeof(struct node));
    if(NULL == ptr)
    {
        printf("\n Node creation failed \n");
        return NULL;
    }
    ptr->status = (char*) malloc(strlen(status) * sizeof(char));
    strcpy(ptr->status, status);
    ptr->path = (char*) malloc(strlen(path) * sizeof(char));
    strcpy(ptr->path, path);
    ptr->background_id = background_id;
    ptr->job_count = g_job_id;
    ptr->next = NULL;
    curr->next = ptr;
    curr = ptr;
    return ptr;
}

/*Will search for a particular node (by process id) in my linked list*/
struct node* search_in_list(pid_t val, struct node **prev)
{
    struct node *ptr = head;
    struct node *tmp = NULL;
    int found = 1;


    while(ptr != NULL)
    {
        if(ptr->background_id == val)
        {
            found = 0;
            break;
        }
        else
        {
            tmp = ptr;
            ptr = ptr->next;
        }
    }

    if(found == 0)
    {
        if(prev)
            *prev = tmp;
        return ptr;
    }
    else
    {
        return NULL;
    }
}
/*Will delete a process from the list once it has completed OR has been killed (using kill() system call)*/
int delete_from_list(pid_t val)
{
    struct node *prev = NULL;
    struct node *del = NULL;
    int last_node = 1;


    del = search_in_list(val,&prev);
    if(del == NULL)
    {
        return -1;
    }
    else
    {
        if(prev != NULL)
            prev->next = del->next;
        
        if ((del == head) && (del == curr))
           last_node = 0;

        if(del == curr)
        {
            curr = prev;
        }
        else if(del == head)
        {
            head = del->next;
        }
        free(del->status);
        free(del->path);
        free(del);
        del = NULL;
        if (last_node == 0)
        {
           head = NULL;
           curr = NULL;
        }
    }

    return 0;
}

/*Will print all background processes (running and stopped) in the background process list (linked list)*/
void print_list(void)
{
    struct node *ptr = head;
    int j = -1;
    while(ptr != NULL)
    {
        if (strcmp(ptr->status, "running") == 0){
	   j++;       
       	   printf("%d[R]: %s\n",ptr->job_count ,ptr->path);
       	}
        else if (strcmp(ptr->status, "stopped") == 0){
	   j++;       
       	   printf("%d[S]: %s\n",ptr->job_count ,ptr->path);
       	}
	 ptr = ptr->next;
	
    }
    if(j > -1)
       	 printf("Total Background jobs: %d \n",j+1);
     
    return;
}

/*Update the status of a process if it has completed*/
void update_process_list(void)
{
    struct node *ptr = head;
    int pstatus;
    pid_t return_pid;

    while(ptr != NULL)
    {
        return_pid = waitpid(ptr->background_id, &pstatus, WNOHANG); /* WNOHANG defined in wait.h*/
        if(return_pid == ptr->background_id)
        {
           strcpy(ptr->status, "RunDone");
        }
        

        ptr = ptr->next;
    }
}

/*Kill a process when given the job number/id*/
void find_and_kill(int j_id)
{
    struct node *ptr = head;
    pid_t return_pid = -1;

    while(ptr != NULL)
    {
        if((ptr->job_count == j_id) && (strcmp(ptr->status, "running") == 0))
        {
           kill(ptr->background_id, SIGTERM);
           return_pid = ptr->background_id; 
           break;
        }
        ptr = ptr->next;
    }
    /*If a process has been killed, then make sure to delete if from the process list*/
    if (return_pid != -1)
      delete_from_list(return_pid);
}
/*Given the start command with a single integer parameter, start the background process ONLY if it is in the stopped state*/
void find_and_start(int j_id)
{
    struct node *ptr = head;

    while(ptr != NULL)
    {
        if((ptr->job_count == j_id) && (strcmp(ptr->status, "stopped") == 0))
        {
           kill(ptr->background_id, SIGCONT);
           strcpy(ptr->status, "running");
	   break;
        }
	/*Else if already running, print an error message*/
        else if((ptr->job_count == j_id) && (strcmp(ptr->status, "running") == 0)){
	   printf("Error: Process [%d] is already running.\n",j_id );
	   break;
	}
        ptr = ptr->next;
    }
}

/*Given the stop command with a single integer parameter, stop the background process ONLY if it is still running*/
void find_and_stop(int j_id)
{
    struct node *ptr = head;

    while(ptr != NULL)
    {
        if((ptr->job_count == j_id) && (strcmp(ptr->status, "running") == 0))
        {
           kill(ptr->background_id, SIGSTOP);
           strcpy(ptr->status, "stopped");
	   break;
        }
	/*Else if already stopped, print an error message*/
        else if((ptr->job_count == j_id) && (strcmp(ptr->status, "stopped") == 0)){
	   printf("Error: Process [%d] has already been stopped.\n",j_id );
	   break;
	}
        ptr = ptr->next;
    }
}
/*Check is any processes in the background process list have completed (status "RunDone"). If the user hits return in th command line (with no arguments) then they will be notified of all processes which have completed.*/
void check_if_process_done(void)
{
    struct node *ptr = head;
    pid_t return_pid;

    while(ptr != NULL)
    {
        return_pid = ptr->background_id;
        if(strcmp(ptr->status, "RunDone") == 0)
        {
           printf("[%d]+  Done		\n",  ptr->job_count);
           delete_from_list(return_pid);
        }

        ptr = ptr->next;
    }

    return;
}
/*Check of the argument given by the user 'isBuiltIn' (meaning the command can be executed in the foreground)*/
int isBuiltIn(char** arguments){
	/*List of allowable built in commands*/
	if((strcmp(arguments[0], "pwd") == 0) |
           (strcmp(arguments[0], "cd") == 0) |
           (strcmp(arguments[0], "bgkill") == 0) |
           (strcmp(arguments[0], "start") == 0) |
           (strcmp(arguments[0], "stop") == 0)
          )
		return true;
	return false;
}
/*Check if the argument is bg*/
int isBackgroundJob(char** arguments){
	if(strcmp(arguments[0], "bg") == 0)
		return true;
	return false;

}

/* main */
int main( void ) {

    char* cmd = NULL;
    char** arguments = NULL;
    int strcount = 0;
    g_job_id = 0;
    while(1){
   	 if (head != NULL){
           //If the list is not empty then update it (job status etc.) 
	   update_process_list();
    	}else{
	   //Reset job count if all processes have completed    	
	   g_job_id = 0;
	}
        char 	*currentDir = getcwd(NULL, 0);
        char	*a = ">";
        strcat(currentDir, a);

	//If there are contents in char ** arguments free it
	//Check strcount which keeps track of length of argument
        if (strcount != 0)
        {
           free (arguments);
           arguments = NULL;
           strcount = 0;
        }
        
        cmd = readline(currentDir);
	//Add user input history to command line
	add_history(cmd);
	//Must free memory allocated by getcwd()
        free(currentDir);
//If the command entered is not empty continue
//Do not go in here if the user hits enter in the command line
if (cmd && *cmd)
{
   //Exit the shell gracefully
   if(strcmp(cmd,"exit") == 0)
   {
      exit(0);
   }
   else if(strcmp(cmd,"bglist") == 0)
   {
      print_list();
   }
   /*If the command is bg with no other argument - do not come in here. That is an invalid argument*/
   else if (strcmp(cmd, "bg") != 0)
   {
	/* Tokenize string */
        char* tokenized;	
        tokenized = strtok(cmd, " ");
        while(tokenized != NULL){
                arguments = (char **)realloc(arguments, (strcount + 1) * sizeof(char *));
                arguments[strcount++] = tokenized;
                tokenized = strtok(NULL, " ");
        }
        arguments = (char **)realloc(arguments, (strcount + 1) * sizeof(char *));
        arguments[strcount] = NULL;

        //Dealing with built in commands
        if (isBuiltIn(arguments)){
		//If user wishes to print the working directory
                if(strcmp(arguments[0], "pwd") == 0){

                        char * cwd;
                        cwd = getcwd(NULL, 0);

                        printf("%s\n", cwd);
                        free(cwd);

                }else if(strcmp(arguments[0], "cd") == 0){
			//Change the current directory
                        chdir(arguments[1]);
                }else if(strcmp(arguments[0], "bgkill") == 0){
			//Kill job with specified job number in arguments[1]
                        find_and_kill(atoi(arguments[1]));
			//Notify user of killed/terminated process
			printf("[%s]+  Terminated	\n", arguments[1]);
                }else if(strcmp(arguments[0], "start") == 0){
			//Start a stopped background job
                        find_and_start(atoi(arguments[1]));
                }else if(strcmp(arguments[0], "stop") == 0){
			//Stop a running background job
                        find_and_stop(atoi(arguments[1]));
                }

	} else if((strcmp(arguments[0], "ls") == 0) | (strcmp(arguments[0], "bg") == 0))
        {
		//Argument is either ls / bg and they require forking
		//Safe to continue
                pid_t child_pid = fork();
		//Execute ls / bg in the child process
                if (child_pid == 0){
		   //In the child process
                   if(strcmp(arguments[0], "ls") == 0){		
                      execvp(arguments[0], arguments);
                    }
                     else if(strcmp(arguments[0], "bg") == 0){
			   execvp(arguments[1], &arguments[1]);
                    }
                } else {
		    //In the parent process
                    char *path_bg_root = getcwd(NULL, 0);
                    if (child_pid == (pid_t)(-1)) {
                       fprintf(stderr, "Fork failed.\n"); exit(1);
                    }
                    else {
                       bg_id = getpid();
		       //If background job, add to the process list, and do not wait for process to finish. Continue in parent.
                       if(isBackgroundJob(arguments)){
                          add_to_list("running", path_bg_root, (long) child_pid);
			  g_job_id++;
                       } else {
			//Else not a background process, so wait for child to finish!! (ls)
                        wait(NULL);
                       }

                    }
		    //Must free memory allocated my getcwd()
                    free(path_bg_root); 
                }
        }

   }//End of else if not bg (we don't wait to execute the above code for a single arg of 'bg')
  } else {
     //command entered is empty (user hit enter), check if any processes have completed
     check_if_process_done();

  }
	//free memory allocated by readline
	free(cmd);
 }//End of while
    // Free the memory allocated by the line entered by the user
     if (strcount != 0)
     {
        free (arguments);
        arguments = NULL;
        strcount = 0;
     }
}//End of main
