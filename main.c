#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>
#include "a3Functions.h"

char **stringToArray(char *string, int *ptr, char **fileName, int *chgStdOut, int *chgStdIn, int *backgroundIndicator)
{

	// the number of string addresses in the array starts at 1
	// so the array of string addresses only has room for 1 string
	int numOfStrings = 1;

	// creates an array of string addresses, so an array of strings
	// with the size of one string address
	char **arr = malloc(numOfStrings * sizeof(char *));
	// first parse of the string passed in
	char *token = strtok(string, " ");
	token = variableExpansion(token);

	// this will indicate if the character # was the first part of the input
	if (token[0] == '#')
	{
		*ptr = 1;
	}

	int index = 0;

	// parse the string until it reaches the null
	while (token != NULL)
	{
		// first check to see if change stdout symbol is the current token
		if (strcmp(token, ">") == 0)
		{
			// if so, check the next token to make sure it is not Null
			if ((token = strtok(NULL, " ")) != NULL)
			{
				// add token to filename
				fileName[0] = token;
				// indicate requirement to change std out so child process later can do that
				*chgStdOut = 0;
			}
			else
			{
				printf("\nThere is no fileName\n");
			}
			// this is checking to see if the change stdin symbol is there
		}
		else if (strcmp(token, "<") == 0)
		{
			// if so, check the next token value to make sure it exists and put it in file name string
			if ((token = strtok(NULL, " ")) != NULL)
			{
				fileName[1] = token;
				// changes standin indicator so child process knows to change input before running exec function
				*chgStdIn = 0;
			}
			else
			{
				printf("\nThere is no fileName\n");
			}
		} /*else if(strcmp(token, "&") == 0) {
			*backgroundIndicator = 0;
		}*/
		// add tokens to array
		else
		{
			// place the address of the parsed string into the arr of string addresses
			arr[index] = token;
			index++;

			// checks to see if there are more string addresses than our array of strings can hold
			if (index >= numOfStrings)
			{
				// increase the number of string addresses the array of strings can hold by 1
				numOfStrings++;
				arr = realloc(arr, numOfStrings * sizeof(char *));
			}
		}
		// checks for null here because it might already be null if the it read a > or <, since token is used again there to check
		// for filename, and if it doesn't exist then token will just be null
		if (token != NULL)
		{
			token = strtok(NULL, " ");
		}

		// checks each string for variable expansion
		if (token != NULL)
		{
			token = variableExpansion(token);
		}
	}
		// this looks at the last entered token after the loop has ended and checks to see if its the & symbol
		if (strcmp(arr[index-1], "&") == 0)
		{
			// if it is then set backgroundIndicator to 0
			*backgroundIndicator = 0;
			// set where the & symbol was to NULL so it is not included in the array
			arr[index-1] = NULL;

		}
		// at the end, we will have increases our array of strings by 1, so we now fill the last index with the NULL character
		else
		{
			arr[index] = NULL;
		}
		
	//arr[index] = NULL;
	return arr;
}

int main(void)
{
	size_t size = 2048;
	pid_t childPid = -1;
	int childStatus = 0;
	int fileDescriptorWrite;
	int fileDescriptorRead;
	int initiateExitStatus = -1;
	int backgroundIndicator = -1;
	char *usrInput = calloc(size, sizeof(char));

	// fileName[0] is for stdOut file name, and fileName[1] is for stdIn file name
	char **fileName = malloc(2 * sizeof(char *));

	int commentCheck = 0;

	// infinite loop
	while (1)
	{

		printf("\n: ");
		int result = getline(&usrInput, &size, stdin);

		// printf("Here is your input:%d\n", result);

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
			char **argv = stringToArray(usrInput, &commentCheck, fileName, &standOut, &standIn, &backgroundIndicator);

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
						// Child process executes this branch
						execvp(argv[0], argv);
						perror("\ncommand Not Found");
						exit(1);
					}
					else
					{
						
						// The parent process executes this branch
						childPid = waitpid(childPid, &childStatus, 0);
						
						/*
						if (WIFEXITED(childStatus))
						{
							printf("\nChild %d exited normally with status %d\n", childPid, WEXITSTATUS(childStatus));
						}
						else
						{
							printf("\nChild %d exited abnormally due to signal %d\n", childPid, WTERMSIG(childStatus));
						}
						printf("\nIn the parent process waitpid returned value %d\n", childPid);
						*/
					}
				}
				// reset values
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