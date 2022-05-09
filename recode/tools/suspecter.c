#include <stdlib.h> /* exit func */
#include <stdio.h> /* printf func */
#include <fcntl.h> /* open syscall */
#include <getopt.h> /* args utility */
#include <sys/ioctl.h> /* ioctl syscall*/
#include <unistd.h> /* close syscall */
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/wait.h>

#define MAX_LEN 256

#define REGISTER_PATH "/proc/recode/security/suspect"

int main(int argc, char **argv)
{
	char *name = argv[1];
	char **args = (argv + 1);

	if (argc < 2)
		goto end;

	/** Registration phase */
	int child_pid = getpid();
	FILE *file = fopen(REGISTER_PATH, "w");

	if (!file) {
		printf("Cannot open %s. Exiting...\n", REGISTER_PATH);
		exit(-1);
	}

	fprintf(file, "%d", child_pid);
	fclose(file);

	printf("[SUSPECTER] Registered PID %d\n", child_pid);

	execvp(name, args);
	while(1);

end:
	return 0;
}