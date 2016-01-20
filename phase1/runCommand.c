#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#define MILL 1000000
#define THOU 1000

long compute_wall_clock_time(struct timeval time_start, struct timeval time_end);
void print_child_stats(struct rusage child_usage, struct timeval time_start, struct timeval time_end);

int main(int argc, char *argv[]) {
	// printf("I'm process with pid = %d\n", getpid());

	pid_t child_pid, tpid;
  int child_status;
	struct rusage child_usage;
	struct timeval time_start, time_end;

	gettimeofday(&time_start, NULL);
	child_pid = fork();
	// printf("child_pid = %d\n", child_pid);
	if (child_pid == 0) { // this is done by the child process
		// printf("I'm the child process pid = %d, ppid = %d\n", getpid(), getppid());
		execvp(argv[1], &argv[1]);
		printf("Unknown command\n"); // if execvp returns, it must have failed
		exit(1);
	} else { // this is done by the parent
			// gettimeofday(&time_start, NULL);
		  waitpid(child_pid, &child_status, 0); // wait for child process to terminate
			if (WIFEXITED(child_status)) { // if the child process ends normally
				gettimeofday(&time_end, NULL);
				// printf("Wall-clock time = %ld\n", (long) end.tv_usec - start.tv_usec);
				getrusage(RUSAGE_CHILDREN, &child_usage);
				print_child_stats(child_usage, time_start, time_end);
				exit(0);
		  }
	}
}

// compute wall clock time in milliseconds
long compute_wall_clock_time(struct timeval time_start, struct timeval time_end) {
	long start_usec = time_start.tv_sec * MILL + time_start.tv_usec;
	long end_usec = time_end.tv_sec * MILL + time_end.tv_usec;
	return (end_usec - start_usec) / THOU;
}

// print all statistics for the child process
void print_child_stats(struct rusage child_usage, struct timeval time_start, struct timeval time_end) {
	long wall_clock_time = compute_wall_clock_time(time_start, time_end);
	long user_cpu_time = (child_usage.ru_utime.tv_sec * THOU) + (child_usage.ru_utime.tv_usec / THOU);
	long system_cpu_time = (child_usage.ru_stime.tv_sec * THOU) + (child_usage.ru_stime.tv_usec / THOU);
	long num_page_faults = child_usage.ru_minflt + child_usage.ru_majflt;

	// print all information
	printf("Wall-clock time = %li milliseconds.\n", wall_clock_time);
	printf("User CPU time = %li milliseconds.\n", user_cpu_time);
	printf("System CPU time = %li milliseconds.\n", system_cpu_time);
	printf("Voluntary context switches = %li.\n", child_usage.ru_nvcsw);
  printf("Involuntary context switches = %li.\n", child_usage.ru_nivcsw);
	printf("Page faults = %li.\n", num_page_faults);
	printf("Page faults that could be satisfied with unreclaimed pages = %li.\n", child_usage.ru_majflt);
}
