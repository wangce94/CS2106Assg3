#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 3)
	{
		printf("\nUsage: %s <file to check>\n", av[0]);
		printf("Prints: 'R' = Read only, 'W' = Read/Write\n\n");
		return -1;
	}
	
	initFS("part.dsk", "123");
	unsigned int attr = getattr(av[1]);
	if(attr == FS_FILE_NOT_FOUND) {
		printf("FILE NOT FOUND\n");
		closeFS();
		return -1;
	}
	
	if(attr & 0b100) {
		printf("R\n");
	} else {
		printf("W\n");
	}
	
	closeFS();

	return 0;
}
