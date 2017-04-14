#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 3)
	{
		printf("\nUsage: %s <file to check set attrbute> <attribute>\n", av[0]);
		printf("Attribute: 'R' = Read only, 'W' = Read/Write\n\n");
		return -1;
	}

	initFS("part.dsk", "nothing");

    unsigned int attr = getattr(av[1]);

    if (attr == FS_FILE_NOT_FOUND) {
    	printf("\nFILE NOT FOUND\n");
    	closeFS();
    	return -1;
    }


    if (strcmp(av[2], "R") == 0 || strcmp(av[2], "r")) {
        attr |= 0b100;
    } else if (strcmp(av[2], "W") == 0 || strcmp(av[2], "w")) {
    	attr &= 0b011;
    } else {
    	printf("\nWrong attribute!\n");
    }

    setattr(av[1], attr);

    closeFS();

	return 0;
}
