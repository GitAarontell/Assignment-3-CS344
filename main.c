#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>
#include "a3Functions.h"

int mode = 0;
/*
 * sets every value in array passed in to -10, used for maintaining array of pid values
 */
void setArr(int arr[], int length)
{
	for (int i = 0; i < length; i++)
	{
		arr[i] = -10;
	}
}

/*
 * adds pid value to array passed in
 */
void addToArr(int arr[], int num, int length)
{
	// adds pid value to any empty part of the array which will be indicated
	// as empty by having the value -10
	for (int i = 0; i < length; i++)
	{
		if (arr[i] == -10)
		{
			arr[i] = num;
			break;
		}
	}
}

/*
 * searches for pids that are finished and prints out information when they are finished
 */
void searchFinishedPIDs(int arr[], int length)
{
	// declaring variables for waitpid
	pid_t bgChildPid = -100;
	int bgChildStatus = -100;

	// looks through the array containing pids
	for (int i = 0; i < length; i++)
	{
		// if any value is not -10 then that is a pid we are waiting for
		if (arr[i] != -10)
		{
			// set bgChildPid to that pid so we can check if it is done
			bgChildPid = arr[i];
			bgChildPid = waitpid(bgChildPid, &bgChildStatus, WNOHANG);

			// if waitpid returns 0 then it is done
			if (bgChildPid != 0)
			{
				if (WIFEXITED(bgChildStatus))
				{
					printf("\nbackground pid %d is done: exit value %d\n", bgChildPid, WEXITSTATUS(bgChildStatus));
				}
				else
				{
					printf("\nbackground pid %d is done: terminated by signal %d\n", bgChildPid, WTERMSIG(bgChildStatus));
				}

				// reset arrays value to -10, since process with that PID is done
				arr[i] = -10;
			}
		}
	}
}

void handleSIGTSTP(int signo)
{
	char *entering = "\nEntering foreground-only mode (& is now ignored)\n";
	char *exiting = "\nExiting foreground-only mode\n";
	if (mode == 0) {
		write(STDOUT_FILENO, entering, 31);
		mode = -1;
	} else if (mode == -1) {
		write(STDOUT_FILENO, exiting, 51);
		mode = 0;
	}
}

int main(void)
{
	// for foreground processes, kept seperate from background
	// process variables to ensure status works everytime
	size_t size = 2048;
	pid_t childPid = -1;
	int childStatus = 0;

	// background process variables
	pid_t bgChildPid = -100;
	int bgChildStatus = -100;
	int PIDArr[100];
	setArr(PIDArr, sizeof(PIDArr) / sizeof(int));

	// variables that deal with opening and closing files
	int fileDescriptorWrite;
	int fileDescriptorRead;
	int initiateExitStatus = -1;
	int backgroundIndicator = -1;

	struct sigaction ignore_action = {0}, SIGTSTP_action = {0};
	ignore_action.sa_handler = SIG_IGN;
	sigaction(SIGINT, &ignore_action, NULL);
	
	SIGTSTP_action.sa_handler = handleSIGTSTP;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	// fileName[0] is for stdOut file name, and fileName[1] is for stdIn file name
	char **fileName = malloc(2 * sizeof(char *));

	// allocate memory for usrinput
	char *usrInput = calloc(size, sizeof(char));

	// used to check if a line is a comment of not
	int commentCheck = 0;

	// infinite loop
	while (1)
	{

		printf("\n: ");
		int result = getline(&usrInput, &size, stdin);

		// getline returns 1 when there is no input, so if there is input then process string
		if (result != 1)
		{

			// gets rid of the \n at the end that gets returned with getline
			noEndLine(usrInput);

			// these are used in stringToArray to check whether there is a < or > and if so sets them to 0
			// for use by handleRedirection in child process
			int standOut = -1;
			int standIn = -1;

			// puts all the strings into a string array args
			char **argv = stringToArray(usrInput, &commentCheck, fileName, &standOut, &standIn, &backgroundIndicator, mode);

			// if commentCheck is not equal to 0 then skip
			if (commentCheck == 0)
			{
				// this will open files for read or write in the parent process
				checkFileOpen(fileName[0], fileName[1], standOut, standIn, &fileDescriptorWrite, &fileDescriptorRead, &initiateExitStatus, backgroundIndicator);

				if (strcmp(argv[0], "cd") == 0)
				{
					handleDirChange(argv[1]);
				}
				else if (strcmp(argv[0], "status") == 0)
				{
					// printf("\nthis is status: %s\n", argv[0]);

					printStatus(childPid, WEXITSTATUS(childStatus));
				}
				else if (strcmp(argv[0], "exit") == 0)
				{
					// make sure to add functionality to terminate all processes before exiting shell!!!!!!!!!!!!!!!!!!!!!!!!!!!
					handleExit(PIDArr);
				}
				else
				{
					childPid = fork();
					if (childPid == -1)
					{
						perror("fork() failed!");
						exit(1);
						// this will run if an open file fails, and will set exit status to 1 without exiting shell
					}
					else if (childPid == 0 && initiateExitStatus == 1)
					{
						exit(1);
					}
					else if (childPid == 0)
					{
						// checkFileOpen(fileName[0], fileName[1], standOut, standIn, &fileDescriptorWrite, &fileDescriptorRead);
						handleRedirection(standOut, standIn, fileDescriptorWrite, fileDescriptorRead, backgroundIndicator);
						sigaction(SIGTSTP, &ignore_action, NULL);
						// if background indicator is not active then change sigint to default result
						if (backgroundIndicator != 0)
						{
							ignore_action.sa_handler = SIG_DFL;
							sigaction(SIGINT, &ignore_action, NULL);
						}
						// Child process executes this branch
						execvp(argv[0], argv);
						perror("\ncommand Not Found");
						exit(1);
					}
					else
					{
						if (backgroundIndicator == 0)
						{
							// set bgChild to childPiD
							bgChildPid = childPid;

							printf("\nbackground pid is %d", bgChildPid);

							bgChildPid = waitpid(bgChildPid, &bgChildStatus, WNOHANG);

							// if background process hasn't finished then add pid to array
							if (bgChildPid == 0)
							{
								addToArr(PIDArr, childPid, sizeof(PIDArr) / sizeof(int));
							}
							else
							{
								printf("\nbackground pid: %d is done: exit value %d", bgChildPid, WEXITSTATUS(bgChildStatus));
							}
						}
						else
						{
							// The parent process executes this branch
							childPid = waitpid(childPid, &childStatus, 0);
						}
						// will print out signal that caused foreground child to exit
						if (WIFSIGNALED(childStatus))
						{
							printf("\nterminated by signal %d\n", WTERMSIG(childStatus));
							// !!!!!!! solution to exit, put the childPid that terminate abnormally in another array, the only
							// background processes are background ones still running, which are already stored, and then ones
							// terminated abnormally which is shown here, run through them killing them one by one
							childStatus = 0;
						}
					}
				}
				// reset values
				searchFinishedPIDs(PIDArr, sizeof(PIDArr) / sizeof(int));
				backgroundIndicator = -1;
				initiateExitStatus = -1;
				standOut = -1;
				standIn = -1;
				free(argv);
			}
		}
		// reset comment check
		commentCheck = 0;
	}
	free(usrInput);

	return 0;
}