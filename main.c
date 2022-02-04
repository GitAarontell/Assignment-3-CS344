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

	//noEndLine(path);

	printf("\n%s\n", getcwd(s, 100));

	// if the chdir does not return 0 then error
	if (chdir(path) != 0)
	{
		printf("No such file or directory\n");
	}
	printf("%s\n", getcwd(s, 100));
}

void processTokensToString(char *token, char *stringArr[], int *index)
{
	//noEndLine(token);
	char *s = variableExpansion(token);
	stringArr[*index] = token;
	*index++;
	printf("\nThis is index %d\n", *index);
}

void correctlySizeArr(char *arr1[], char *arr2[], int *index) {

}

int main(void)
{
	
	size_t size = 2048;
	char *token;
	char *usrInput = calloc(size, sizeof(char));

	// infinite loop
	while (1)
	{

		printf(": ");
		getline(&usrInput, &size, stdin);
		// printf("you entered: %s strleng: %lu this is weird\n", usrInput, strlen(usrInput));

		noEndLine(usrInput); //added just now

		token = strtok(usrInput, " ");
		//noEndLine(token);

		if (strcmp(token, "cd") == 0 || strcmp(token, "cd\n") == 0)
		{
			token = strtok(NULL, " ");
			if (token == NULL)
			{
				changeDir("~");
			}
			else
			{
				changeDir(token);
			}
			// for other programs we want to run execlp list of string, and p is search the paths
			// this will end parsing loop
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
				char *arr[512];
				int index = 0;
				int *idxP = &index;

				while (token != NULL) {
					processTokensToString(token, arr, idxP);
					token = strtok(NULL, " ");
				}

				// we know the number of arguments based on how many times token runs

				// i need to loop through all the tokens and turn them into an array of strings, then pass it into the excelp
				// Child process executes this branch
				// execlp();
				return 0;
			}
			else
			{
				// The parent process executes this branch
				childPid = waitpid(childPid, &childStatus, 0);
				printf("In the parent process waitpid returned value %d\n", childPid);
			}
		}
	}
	

	return 0;
}