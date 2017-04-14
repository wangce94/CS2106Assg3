#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 3)
	{
		printf("\nUsage: %s <file to check out> <password>\n\n", av[0]);
		return -1;
	}
	FILE *fp = fopen(av[1], "w");
	
	initFS("part.dsk", av[2]);
	int fileIndex = openFile(av[1], MODE_READ_ONLY);
	if(_result == FS_FILE_NOT_FOUND) {
		printf("FILE NOT FOUND\n");
		closeFS();
		return -1;
	}
	if(fileIndex == -1) {
		printf("ERROR\n");
		closeFS();
		return -1;
	}
	char* buffer[1024];
	_result = 1;
	while(_result != 0) {
		readFile(fileIndex, buffer, sizeof(char), 1024);
		fwrite(buffer, sizeof(char), _result, fp);
	}
	
	fclose(fp);
	closeFile(fileIndex);
	closeFS();
	
	return 0;
}
