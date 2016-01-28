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

long compute_wall_clock_time(struct timeval time_start, struct timeval time_end);
void print_child_stats(struct rusage child_usage, struct timeval time_start, struct timeval time_end);
int check_command(char *command, char *command_arr[]);
struct rusage subtract_previous_usage(struct rusage previous_usage, struct rusage child_usage);
int execute_command(char *command_arr[]);
// void print_struct(struct rusage astruct);

struct rusage previous_usage;
int command_index = 0; 

int main(int argc, char *argv[]) {
	char command[MAX_CHAR + 1];
	int error_code;
	char *command_arr[MAX_ARG + 1];
	
	
	while (fgets(command, sizeof(command), stdin) != NULL) {
		command[strlen(command) - 1] = '\0';
	  	printf("==> %s\n", command);
	  	
	  	
 		memset(command_arr, 0 , sizeof(char *) * (MAX_ARG + 1));

		// check for error
		error_code = check_command(command, command_arr);
		// printf("error code = %d\n", error_code);
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
	
	printf("%s\n", "==> exit");
	return 0;
}

// 
int check_command(char *command, char *command_arr[]) {	
	char *token;
	int num_arg = 0;
	token = strtok(command, " "); // get the first token

	// get the remaining tokens and keep track of number of arguments
	while (token != NULL) { 
		// printf("token = %s\n", token);			
		command_arr[num_arg] = token;
		token = strtok(NULL, " ");
		// printf("command[0] = %s\n", command_arr[0]);
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

	// printf("argument[0] = %s\n", command_arr[0]);

	execute_command(command_arr);
}



int execute_command(char *command_arr[]) {
	pid_t child_pid;
  int child_status;
	struct rusage child_usage, total_usage;
	struct timeval time_start, time_end; 

	if (strcmp(command_arr[0], "cd") == 0) {
		if (chdir(command_arr[1]) == 0) {
			char* buf = malloc(128);
			printf("Current directory is changed to %s\n", getcwd(buf, 128));
			free(buf);
			return 0;
		} else {			
			return INVALID_PATH;
		}
	}

	child_pid = fork();


	// printf("child_pid = %d\n", child_pid);
	if (child_pid == 0) { // this is done by the child process
		// printf("I'm the child process pid = %d, ppid = %d\n", getpid(), getppid());
		execvp(command_arr[0], &command_arr[0]);
		printf("Unknown command\n"); // if execvp returns, it must have failed
		return 1;
	} else { // this is done by the parent
		gettimeofday(&time_start, NULL);
	  	waitpid(child_pid, &child_status, 0); // wait for child process to terminate
		if (WIFEXITED(child_status)) {
			gettimeofday(&time_end, NULL);
			// printf("Wall-clock time = %ld\n", (long) end.tv_usec - start.tv_usec);
			getrusage(RUSAGE_CHILDREN, &child_usage);

			total_usage = child_usage;
			child_usage = subtract_previous_usage(previous_usage, child_usage);
			previous_usage = total_usage;
			
			print_child_stats(child_usage, time_start, time_end);
			return 0;
	  }
	}
}

struct rusage subtract_previous_usage(struct rusage previous_usage, struct rusage child_usage) {
	if (command_index != 0) {
		// printf("SUBTRACT\n");
		child_usage.ru_utime.tv_sec -= previous_usage.ru_utime.tv_sec;
		child_usage.ru_utime.tv_usec -= previous_usage.ru_utime.tv_usec;
		child_usage.ru_stime.tv_sec -= previous_usage.ru_stime.tv_sec;
		child_usage.ru_stime.tv_usec -= previous_usage.ru_stime.tv_usec;
		child_usage.ru_minflt -= previous_usage.ru_minflt; 
		child_usage.ru_majflt -= previous_usage.ru_majflt;
		child_usage.ru_nvcsw -= previous_usage.ru_nvcsw;
		child_usage.ru_nivcsw -= previous_usage.ru_nivcsw;
		child_usage.ru_majflt -= previous_usage.ru_majflt;
	}
	return child_usage;
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
	printf("_________________________________________________________________\n");
	printf("|| Wall-clock time = %li milliseconds.\n", wall_clock_time);
	printf("|| User CPU time = %li milliseconds.\n", user_cpu_time);
	printf("|| System CPU time = %li milliseconds.\n", system_cpu_time);
	printf("|| Voluntary context switches = %li.\n", child_usage.ru_nvcsw);
  printf("|| Involuntary context switches = %li.\n", child_usage.ru_nivcsw);
	printf("|| Page faults = %li.\n", num_page_faults);
	printf("|| Page faults that could be satisfied with unreclaimed pages = %li.\n", child_usage.ru_majflt);
	printf("_________________________________________________________________\n");
}
