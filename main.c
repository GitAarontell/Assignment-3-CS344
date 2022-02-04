#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>

// processes the string
void noEndLine(char *string)
{
	// gets size of string
	int size = strlen(string);

	// if the string has a \n character at the end then get rid of it and change size
	if (string[size - 1] == '\n')
	{
		string[size - 1] = '\0';
		size = size - 1;
	}
}

// handles variable expansion for $$ symbols
char *variableExpansion(char *string)
{

	pid_t pid = getpid();
	int size = strlen(string);
	char *buffer = calloc(20, sizeof(char));
	char numBuffer[20];
	int bufferIdx = 0;

	// convert pid to string
	snprintf(numBuffer, 20, "%d", pid);

	// loops through each character of string
	for (int i = 0; i < size; i++)
	{
		// if current character in string is $ and next character is also $
		if (string[i] == '$' && string[i + 1] == '$')
		{
			// then loop through each character in numBuffer (pid)
			for (int j = 0; j < strlen(numBuffer); j++)
			{
				// add each number to buffer
				buffer[bufferIdx] = numBuffer[j];
				// increment buffer index
				bufferIdx++;
			}
			// increment i here to skip over first $ symbol
			// then the for loop will increment i again to skip over second $ symbol
			i++;

			// continue immediatly starts next loop
			continue;
		}

		// add current char in string to buffer string
		buffer[bufferIdx] = string[i];
		bufferIdx++;
	}
	// printf("\n%s length: %lu  pid: %s\n", buffer, strlen(buffer), numBuffer);

	// free buffer memory
	return buffer;
}

char **stringToArray(char *string, int *ptr)
{

	// the number of string addresses in the array starts at 1
	// so the array of string addresses only has room for 1 string
	int numOfStrings = 1;

	// creates an array of string addresses, so an array of strings
	// with the size of one string address
	char **arr = malloc(numOfStrings * sizeof(char *));
	// first parse of the string passed in
	char *token = strtok(string, " ");

	// this will indicate if the character # was the first part of the input
	if (token[0] == '#')
	{
		*ptr = 1;
	}

	int index = 0;

	// parse the string until it reaches the null
	while (token != NULL)
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
		// at the end, we will have increases our array of strings by 1, so we now fill the last index with the NULL character
		token = strtok(NULL, " ");

		// checks each string for variable expansion
		if (token != NULL)
		{
			token = variableExpansion(token);
		}
	}
	arr[index] = NULL;
	return arr;
}

void changeDir(char *path)
{

	char s[100];
	uid_t uid = getuid();
	struct passwd *pwd = getpwuid(uid);

	// getline adds a newline to the end of the string, so to get rid of newline character add \0 to the end
	// only if the path was the last argument passed in the command line

	// gets the home path and sets it to path
	if (strcmp(path, "~") == 0)
	{
		path = pwd->pw_dir;
	}

	printf("\n%s\n", getcwd(s, 100));

	// if the chdir does not return 0 then error
	if (chdir(path) != 0)
	{
		printf("No such file or directory\n");
	}
	printf("%s\n", getcwd(s, 100));
}

void printArr(char **arr)
{
	for (int i = 0; i < sizeof(arr) - 1; i++)
	{
		printf("\n%s\n", arr[i]);
	}
}

int main(void)
{
	size_t size = 2048;
	char *usrInput = calloc(size, sizeof(char));
	int commentCheck = 0;

	// infinite loop
	while (1)
	{

		printf(": ");
		int result = getline(&usrInput, &size, stdin);

		//printf("Here is your input:%d\n", result);

		// getline returns 1 when there is no input, so if there is input then process string
		if (result != 1)
		{

			// gets rid of the \n at the end that gets returned with getline
			noEndLine(usrInput);

			// puts all the strings into a string array args
			char **args = stringToArray(usrInput, &commentCheck);

			// if commentCheck is not equal to 0 then skip
			if (commentCheck == 0)
			{
				//printArr(args);
				if (strcmp(args[0], "cd") == 0)
				{

					if (args[1] == NULL)
					{
						changeDir("~");
					}
					else
					{
						changeDir(args[1]);
					}
				}
				else
				{
					int childStatus = -10;
					pid_t childPid = fork();
					if (childPid == -1)
					{
						perror("fork() failed!");
						exit(1);
					}
					else if (childPid == 0)
					{

						// we know the number of arguments based on how many times token runs

						// i need to loop through all the tokens and turn them into an array of strings, then pass it into the excelp
						// Child process executes this branch

						execvp(args[0], args);
						return 0;
					}
					else
					{
						// The parent process executes this branch
						childPid = waitpid(childPid, &childStatus, 0);
						printf("\nIn the parent process waitpid returned value %d\n", childPid);
					}
				}
				free(args);
			}
		}

		// reset comment check
		commentCheck = 0;
	}
	free(usrInput);

	return 0;
}