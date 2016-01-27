#include <sys/syscall.h>
#include <sys/type.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>

long compute_wall_clock_time(struct timeval before_time, struct timeval after_time);
void print_child_statistics(struct rusage child_stats, struct timeval before_time, struct timeval after_time);

int main(int argc, char* argv[]) {
	struct rusage prev_stats;
	int prev_stats_init = 0;
	while (1){
		printf("==>");
		char input[129];	
		
		if (fgets(input, 129, stdin) == NULL) {
			printf("\n");
			exit(0);
		}
		
		// Missing command to run
		if (strlen(input) == 1 && input[strlen(input) - 1] == "\n"){
			printf("Need a command to run\n");
			continue;
		}

		char* arguments[33];
		char* argument = strtok(input, " \n");
		int i;
		for (i = 0; i <= 32; i++){
			arguments[i] = argument;
			argument = strtok(NULL, " \n");
		}
		
		if (arguments[32] != NULL){
			printf("You have entered more than 32 arguments\n");
			continue;
		}
		
		char* command_name = arguments[0];
		
		if (strcmp(command_name, "exit") == 0){
			exit(0);		
		}

		if (strcmp(command_name, "cd") == 0){
			chdir(arguments[1]);
		}
		
		// Fork a child process and get its PID
		int pid = fork();

		if (pid != 0) {
			// In parent process
			int status;
			struct timeval before_time, after_time;

			// Get the time information before running the command
			gettimeofday(&before_time, NULL);

			// Wait for the child to finish running the command
			waitpid(pid, &status, 0);

			// If the child terminated normally, print the statistics
			if (WEXITSTATUS(status) == 0) {
				// Get the time right after the child process finished
				gettimeofday(&after_time, NULL);
			
				// Get the statistics of the child process that just finished
				struct rusage child_stats;
				getrusage(RUSAGE_CHILDREN, &child_stats);

				// Print the statistics
				print_child_statistics(child_stats, prev_stats, prev_stats_init, before_time, after_time);

				prev_stats = child_stats;
				prev_stats_init = 1;
			}
	
		} else {
			// In the child process
			int result = execvp(command_name, arguments);

			// Check if the command failed
			if (result == -1) {
				// The command failed, so print out he error number and exit
				printf("Invalid command!\nError number: %i\nError message: %s\n", errno, strerror(errno));
				exit(1);
			}
		}

	}
	return 0;
}

// Get the microseconds difference
long get_micro_difference(struct timeval before_time, struct timeval after_time){
	long micro_difference = (long) (after_time.tv_usec - before_time.tv_usec);

	// Correct the difference when the after microseconds > before microseconds
	if (micro_difference < 0) {
		micro_difference += 1000000;
	}
	
	return micro_difference;
}

// Compute the difference between the two specified timevals
long compute_wall_clock_time(struct timeval before_time, struct timeval after_time) {
	// Get the seconds difference and convert to microseconds
	long difference = (long) ((after_time.tv_sec - before_time.tv_sec) * 1000000);

	// Add the corrected microsecond difference to the total
	difference += get_micro_difference(before_time, after_time);

	// Convert to milliseconds and return the result
	return (difference / 1000);
}

// Print the statistics about the child process with the given rusage data
void print_child_statistics(struct rusage child_stats, struct rusage prev_stats, int useage_prev_stats, struct timeval before_time, struct timeval after_time) {
	long difference, user_cpu_time, system_cpu_time, vol
	long difference = compute_wall_clock_time(before_time, after_time);
	long user_cpu_time = (child_stats.ru_utime.tv_sec * 1000) + (child_stats.ru_utime.tv_usec / 1000);
	long system_cpu_time = (child_stats.ru_stime.tv_sec * 1000) + (child_stats.ru_stime.tv_usec / 1000);

	printf("Wall-Clock time: %li milliseconds\n", difference);
	printf("User CPU time: %li milliseconds\n", user_cpu_time);
	printf("System CPU time: %li milliseconds\n", system_cpu_time);
	printf("Voluntary context switches: %li\n", child_stats.ru_nvcsw);
	printf("Involuntary context switches: %li\n", child_stats.ru_nivcsw);
	printf("Page faults: %li\n", child_stats.ru_minflt + child_stats.ru_majflt);
	printf("Page faults that could be satisfied with unreclaimed pages: %li\n", child_stats.ru_majflt);
}
