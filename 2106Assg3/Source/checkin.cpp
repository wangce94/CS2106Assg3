#include "libefs.h"
#define BLOCKSIZE 8192

int main(int ac, char **av)
{
	if(ac != 3)
	{
		printf("\nUsage: %s <file to check in> <password>\n\n", av[0]);
		return -1;
	}

	FILE *fp = fopen(av[1], "r");

	if(fp == NULL)
	{
		printf("\nUnable to open source file %s\n\n", av[1]);
		return -1;
	}

	initFS("part.dsk", av[2]);

    int fileIndex = openFile(av[1], MODE_CREATE);

    if (fileIndex == -1) {
        if (_result == FS_DUPLICATE_FILE) {
            printf("\n%s already exists!\n", av[1]);
        }
        else {
            printf("\nCouldn't check in.\n");
        }
        closeFS();
        return -1;
    }
	
    char block[1024];
    unsigned int dataCount = fread(block, sizeof(char), 1024, fp);

    char block[BLOCKSIZE];
    unsigned int dataCount = fread(block, sizeof(char), BLOCKSIZE, fp);
    

    while (dataCount) {
    	writeFile(fileIndex, block, sizeof(char), dataCount);
    	dataCount = fread(block, sizeof(char), BLOCKSIZE, fp);
    }

    fclose(fp);

    closeFile(fileIndex);

    closeFS();

    return 0;
}
