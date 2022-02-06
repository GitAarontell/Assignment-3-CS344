#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>

int main(void) {
    char *arr[] = {"ls", NULL, "yep"};

    execvp(arr[0], arr);
    return 0;
}