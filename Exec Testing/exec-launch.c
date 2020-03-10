#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <math.h>
#include <process.h>

int main()
{
//Run Bash by creating new processes
char *args[]={"C:\\Windows\\System32\\cmd.exe", NULL};
#ifdef WIN32
	execvp(args[0], args);
	printf("Ending--------");
#else 
	printf("I can only run on Windows!\n");

#endif
return 0;
}