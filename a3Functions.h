#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>

void handleExit(int arr[]) {

	exit(0);
}

void printStatus(pid_t childPid, int status)
{
	if (childPid == -1)
	{
		printf("\nexit value %d\n", status);
	}
	else
	{
		printf("\nexit value %d\n", status);
	}
}

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

char **stringToArray(char *string, int *ptr, char **fileName, int *chgStdOut, int *chgStdIn, int *backgroundIndicator, int mode)
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
			// if mode is 0 then don't set background indicator
			if (mode == 0) {
				*backgroundIndicator = 0;
			}
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

/*
 * changes the directory
 */
void changeDir(char *path)
{

	// s used in printing current working directory to check things
	char s[100];

	uid_t uid = getuid();
	struct passwd *pwd = getpwuid(uid);

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

void handleDirChange(char *path)
{
	if (path == NULL)
	{
		changeDir("~");
	}
	else
	{
		changeDir(path);
	}
}


void checkFileOpen(char *fileNameWrite, char *fileNameRead, int standOut, int standIn, int *fileDescriptorWrite, int *fileDescriptorRead, int *initiateExitStatus, int backgroundIndicator)
{
	if (standOut == 0)
	{
		*fileDescriptorWrite = open(fileNameWrite, O_CREAT | O_TRUNC | O_WRONLY);
		if (*fileDescriptorWrite == -1)
		{
			printf("\nopen() failed on: %s", fileNameWrite);
			// sets exit status to 1 without exiting shell
			*initiateExitStatus = 1;

		}
	} 
	if (standIn == 0)
	{
		*fileDescriptorRead = open(fileNameRead, O_RDONLY);
		if (*fileDescriptorRead == -1)
		{
			printf("\nopen() failed on: %s", fileNameRead);
			// sets exit status to 1 without exiting shell
			*initiateExitStatus = 1;
		}
	}
	// if background indicator is 0 then checks for required changes in files to open
	if (backgroundIndicator == 0){
		if (standOut != 0 && standIn != 0) {
			*fileDescriptorWrite = open("/dev/null", O_RDWR);
			*fileDescriptorRead = *fileDescriptorWrite;
		} else if (standOut == 0 && standIn != 0) {
			*fileDescriptorWrite = open("/dev/null", O_WRONLY);
		} else if (standOut != 0 && standIn == 0) {
			*fileDescriptorRead = open("/dev/null", O_RDONLY);
		}
	}

}

/*
* This functions handles redirecting input and output to point at different file descriptors
*/
void handleRedirection(int standOut, int standIn, int fileDescriptorWrite, int fileDescriptorRead, int backgroundIndicator)
{
	
	int checkOut = 0;
	int checkIn = 0;

	// if standOut value passed in was set to 0 in parsing commandline input, then redirect output
	if (standOut == 0)
	{
		// set output redirection to fileDescriptorWrite, also set in parsing string to array
		checkOut = dup2(fileDescriptorWrite, STDOUT_FILENO);
		// if fails to redirect, print error message and set exit status to 1
		if (checkOut == -1)
		{
			printf("\nFailed to redirect output");
			exit(1);
		}
	} else if (backgroundIndicator == 0) {
		checkOut = dup2(fileDescriptorWrite, STDOUT_FILENO);
		if (checkOut == -1)
		{
			printf("\nFailed to redirect output");
			exit(1);
		}
	}
	// if standIn value passed in was set to 0 in parsing commandline input, then redirect input
	if (standIn == 0)
	{
		// set input redirection to fileDescriptorRead, also set in parsing string to array
		checkIn = dup2(fileDescriptorRead, STDIN_FILENO);
		// if fails to redirect, print error message and set exit status to 1
		if (checkIn == -1)
		{
			printf("\nFailed to redirect input");
			exit(1);
		}
	} else if (backgroundIndicator == 0) {
		checkOut = dup2(fileDescriptorRead, STDIN_FILENO);
		if (checkOut == -1)
		{
			printf("\nFailed to redirect output");
			exit(1);
		}
	}
}
