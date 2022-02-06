#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>

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
