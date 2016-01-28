Project 1
Cuong Nguyen & Huyen Nguyen
chnguyen & hbnguyen

The final shell2 program has every function in shell plus the ability to run background processes. To do this, we create a queue which holds the information about background processes, including PID, a Flag to check whether it needs to be deleted or not, the start and end time.

After entering the command, we check to see if any of the background processes have completed by looping through the queue and run wait4(WNOHANG) for the process pid. If the process is finished, we print the process's information and the statistic from the rusage struct. Then we dequeue the queue. Our program also handle special commands like exit,cd or jobs. When exits, we wait until all background processes are done before exiting the program.

For testing, we run commands inside the shell to make sure that they finish with accurate statistics and also using text files as inputs. Attached are the makefile, runCommand.c, shell.c, shell2.c, a copy of the argument and result for runCommand.c, one input and output of shell.c, and two inputs and two outputs for shell2.c!