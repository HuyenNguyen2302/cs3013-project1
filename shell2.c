// Cuong Nguyen & Huyen Nguyen
// chnguyen@wpi.edu & hbnguyen@wpi.edu
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MILL 1000000
#define THOU 1000
#define MAX_ARG 32
#define MAX_CHAR 128
#define VIOLATE_MAX_ARG 2    // error code 
#define VIOLATE_MAX_CHAR 3   // error code 
#define EMPTY_COMMAND 4 		 // error code
#define INVALID_PATH 5			 // error code
#define TRUE 0
#define FALSE 1

struct process {
	int pid;
	int need_delete;
	struct process *next_process;
	struct rusage child_usage;
	struct timeval time_start, time_end;
};

struct queue {
	struct process *head, *tail;
};

struct rusage previous_usage;
int command_index = 0; 
struct queue * process_queue;

long compute_wall_clock_time(struct timeval time_start, struct timeval time_end);
void print_child_stats(struct rusage child_usage, struct timeval time_start, struct timeval time_end);
int check_command(char *command, char *command_arr[]);
struct rusage subtract_previous_usage(struct rusage previous_usage, struct rusage child_usage);
int execute_command(char *command_arr[], int is_background_process);
void print_all_process();
void run_background_process();
void enqueue_process();
void dequeue_process();
void free_struct(struct process *a_process);
// void print_struct(struct rusage astruct);

int main(int argc, char *argv[]) {
	char command[MAX_CHAR + 1];
	int error_code;
	char *command_arr[MAX_ARG + 1];
	char *command_ptr;
	process_queue = malloc(sizeof(struct queue));
	process_queue->head = process_queue->tail = NULL;
	
	while (1) {
		if (isatty(fileno(stdin))) {
			printf("==> ");
		}
		if (fgets(command, sizeof(command), stdin) != NULL){
			command[strlen(command) - 1] = '\0';
		} else {
			//printf("Can't exit. There are background processes running.\n");
			while (process_queue->head != NULL) {
				run_background_process();
			}
			break;
		}
		if (!isatty(fileno(stdin))) {
	  		printf("==> %s\n", command);
		}
		command_ptr = command;
 		memset(command_arr, 0 , sizeof(char *) * (MAX_ARG + 1));

		// check for error
		run_background_process();
		error_code = check_command(command_ptr, command_arr);
		run_background_process();
		switch(error_code) {
			case VIOLATE_MAX_CHAR:
				printf("%s\n", "ERROR: VIOLATED MAX NUMBER OF CHARACTERS");
				break;
			case VIOLATE_MAX_ARG:
				printf("%s\n", "ERROR: VIOLATED MAX NUMBER OF ARGUMENTS");
				break;
			case EMPTY_COMMAND:
				printf("%s\n", "EMPTY COMMAND");
				break;
			case INVALID_PATH:
				printf("This is not a valid path.\n");
				break;
		}
		command_index++;
	}
	
	memset(process_queue, 0, sizeof(struct queue));
	free(process_queue->head);
	free(process_queue->tail);
	free(process_queue);
	printf("%s\n", "==> exit");
	return 0;
}

// 
int check_command(char *command, char *command_arr[]) {
	//printf("==> %s\n", command);
	int is_background_process = FALSE;
	char *token;
	int num_arg = 0;
	token = strtok(command, " "); // get the first token
	int last_arg = 0;
	int found_last = 0;
	// get the remaining tokens and keep track of number of arguments
	while (token != NULL) { 
		command_arr[num_arg] = token;
		token = strtok(NULL, " ");
		num_arg++;
	}
	
	command_arr[num_arg] = NULL;


	if (num_arg > MAX_ARG) { 		
		return VIOLATE_MAX_ARG;
	}
	
	// check number of charapidcters 
	if (strlen(command) > MAX_CHAR)
		return VIOLATE_MAX_CHAR;

	// check for empty command
	if (strlen(command) == 0)
		return EMPTY_COMMAND;



	if (strcmp(command_arr[num_arg - 1], "&") == 0) {
		is_background_process = TRUE;
		command_arr[num_arg - 1] = '\0';
	}
	execute_command(command_arr, is_background_process);

	return 0;
}


// execute commands after all error checking
int execute_command(char *command_arr[], int is_background_process) {
	pid_t child_pid;
    int child_status;
	struct rusage child_usage;
	struct timeval time_start, time_end; 

	gettimeofday(&time_start, NULL);
	
	// special case 1: cd command
	if (strcmp(command_arr[0], "cd") == 0) {
		if (chdir(command_arr[1]) == 0) {
			printf("Current directory is changed to %s\n", command_arr[1]);
			return 0;
		} else {			
			return INVALID_PATH;
		}
	}

	// special case 2: jobs command
	if (strcmp(command_arr[0], "jobs") == 0) {
		print_all_process();
		return 0;
	}

	// special case 3: exit command
	if (strcmp(command_arr[0], "exit") == 0) {
		if (process_queue->head == NULL) {
			printf("Every's done. Bye.\n");
			exit(0);	
			printf("WTF?\n");
		} else {
			printf("Can't exit. There are background processes running.\n");
		}
		return 0;
	}
 
 	// other cases:
	child_pid = fork();


	// printf("child_pid = %d\n", child_pid);
	if (child_pid == 0) { // this is done by the child process
		execvp(command_arr[0], &command_arr[0]);
		printf("Error occured: %s\n", strerror(errno));
		 // if execvp returns, it must have failed
		exit(1);
	} else { // this is done by the parent

		//run_background_process();
		
		if (is_background_process == FALSE) {
			wait4(child_pid, &child_status, 0, &child_usage);
			if (child_status == 0) {
				struct timeval time_end;
				gettimeofday(&time_end, NULL);
				
				print_child_stats(child_usage, time_start, time_end);
			}
			// TODO: print child stat
		} else {
				struct process *new_process  = malloc(sizeof(struct process));

				new_process->pid = child_pid;

				new_process->need_delete = FALSE;
				new_process->next_process = NULL;
				new_process->time_start = time_start;
				enqueue_process(new_process);
				printf("pid = %d\n", child_pid);
//			}			
		}
	  	return 0;
		// if (WIFEXITED(child_status)) {
		// 	gettimeofday(&time_end, NULL);
		// 	print_child_stats(child_usage, time_start, time_end);
	 //    } 
	}
}

void print_all_process() {
	struct process *current = process_queue->head;
	printf("LIST OF ALL PROCESSES:\n");
	while (current != NULL) {
		printf("\tpid = %d\n", current->pid);
		current = current->next_process;
	}
}

void printQueue() {
	struct process *current = process_queue->head;
	while (current != NULL) {
		printf("%d ", current->pid); 
		current = current->next_process;
	}
	printf("\n");
}

void run_background_process() {
	int stat_loc;
	struct process *current = process_queue->head;
	struct process *next = current ? current->next_process : NULL;
//	printf("In run_background_process\n");
//	print_all_process();
	
	while (current != NULL) {
		int finish = wait4(current->pid, &stat_loc, WNOHANG, &(current->child_usage));
		if (finish > 0) {
			gettimeofday(&(current->time_end), NULL);
			printf("STATS AVAILABLE for pid = %d:\n", current->pid);
			print_child_stats(current->child_usage, current->time_start, current->time_end);
			current->need_delete = TRUE;
			dequeue_process();
			//printQueue();
		}
		current = next;
		next = current ? current->next_process : NULL;
	}
}

void dequeue_process() {
	struct process *current = process_queue->head;
	struct process *pre_current = NULL;
	struct process *to_be_deleted = NULL;

	// case 1: process to be removed is the head of the queue
	if ( (*(process_queue->head)).need_delete == TRUE ) {
		to_be_deleted = process_queue->head;
		process_queue->head = process_queue->head->next_process;
		if (process_queue->tail == to_be_deleted) {
			process_queue->tail = NULL;
		}
		// free_struct(&(process_queue->head)) ?
		free_struct(&(*to_be_deleted));
		return;
	}

	// case 2: process to be removed is the tail of the queue
	if ( (*(process_queue->tail)).need_delete == TRUE ) {
		while (current != NULL) {
			if (current->next_process == process_queue->tail) {
				free_struct(process_queue->tail);
				process_queue->tail = current;
				current->next_process = NULL;
				return;
			}
			current = current->next_process;
		}
	}

	// case 3: process to be removed lies somewhere in the queue
	while (current != NULL) {
		if (current->need_delete == TRUE) {
			pre_current->next_process = current->next_process;
			free_struct(current);
			break;
		}
		pre_current = current;
		current = current->next_process;
	}
}

void enqueue_process(struct process *new_process) {
	
	if (process_queue->tail == NULL) {
		process_queue->head = new_process;
		process_queue->tail = new_process;
	} else {
		process_queue->tail->next_process = new_process;
		process_queue->tail = new_process;
	}
			//printQueue();
}

// void print_struct(struct rusage child_usage) {
// 	long num_page_faults = child_usage.ru_minflt + child_usage.ru_majflt;
// 	printf("Page faults TEST = %li.\n", num_page_faults);
// }

// compute wall clock time in milliseconds
long compute_wall_clock_time(struct timeval time_start, struct timeval time_end) {
	long start_usec = time_start.tv_sec * MILL + time_start.tv_usec;
	long end_usec = time_end.tv_sec * MILL + time_end.tv_usec;
	return (end_usec - start_usec) / THOU;
}

void print_child_stats(struct rusage child_usage, struct timeval time_start, struct timeval time_end) {
	long wall_clock_time = compute_wall_clock_time(time_start, time_end);
	long user_cpu_time = (child_usage.ru_utime.tv_sec * THOU) + (child_usage.ru_utime.tv_usec / THOU);
	long system_cpu_time = (child_usage.ru_stime.tv_sec * THOU) + (child_usage.ru_stime.tv_usec / THOU);
	long num_page_faults = child_usage.ru_minflt + child_usage.ru_majflt;

	// print all information
	printf("\n\tWall-clock time = %li milliseconds.\n", wall_clock_time);
	printf("\tUser CPU time = %li milliseconds.\n", user_cpu_time);
	printf("\tSystem CPU time = %li milliseconds.\n", system_cpu_time);
	printf("\tVoluntary context switches = %li.\n", child_usage.ru_nvcsw);
    printf("\tInvoluntary context switches = %li.\n", child_usage.ru_nivcsw);
	printf("\tPage faults = %li.\n", num_page_faults);
	printf("\tPage faults that could be satisfied with unreclaimed pages = %li.\n", child_usage.ru_majflt);
	printf("_________________________________________________________________\n\n");
}

void free_struct(struct process *a_process) {
	//free(a_process->next_process);
	free(a_process);
	return;
}
