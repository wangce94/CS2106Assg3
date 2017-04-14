#include "libefs.h"

// FS Descriptor
TFileSystemStruct *_fs;

// Open File Table
TOpenFile *_oft;

// Open file table counter
int _oftCount=0;

// Mounts a paritition given in fsPartitionName. Must be called before all
// other functions
void initFS(const char *fsPartitionName, const char *fsPassword)
{
	mountFS(fsPartitionName, fsPassword);
	_fs = getFSInfo();
	_oft = new TOpenFile[_fs->maxFiles]();
}

// Opens a file in the partition. Depending on mode, a new file may be created
// if it doesn't exist, or we may get FS_FILE_NOT_FOUND in _result. See the enum above for valid modes.
// Return -1 if file open fails for some reason. E.g. file not found when mode is MODE_NORMAL, or
// disk is full when mode is MODE_CREATE, etc.

int openFile(const char *filename, unsigned char mode)
{
	int oftIndex = -1;
	//check is there is space to open another file
	int i;
	for(i = 0; i < _fs->maxFiles; i++) {
		if(_oft[i].taken == 0) {
			oftIndex = i;
			break;
		}
	}
	if(oftIndex == -1) {
		return -1;
	}

	unsigned int fileLocation = findFile(filename);
	if(fileLocation == FS_FILE_NOT_FOUND && (mode == MODE_NORMAL || mode == MODE_READ_ONLY)) {
		return -1;
	}
	

	unsigned long *inode_buffer = NULL;
	if(fileLocation == FS_FILE_NOT_FOUND && (mode == MODE_CREATE || mode == MODE_READ_APPEND)) {
		//make directory 
		unsigned int directoryIndex = makeDirectoryEntry(filename, 0, 0);
		//find free descriptor
		inode_buffer = makeInodeBuffer();
		//enter attributes
		unsigned long freeBlock = findFreeBlock();
		//mark block as busy
		markBlockBusy(freeBlock);
		setBlockNumInInode(inode_buffer, 0, freeBlock);
		fileLocation = directoryIndex;
		//find free slot in directory
		//enter name/pointer
	}

	if(fileLocation != FS_FILE_NOT_FOUND && (mode == MODE_NORMAL || mode == MODE_READ_ONLY)) {
		inode_buffer = makeInodeBuffer();
		loadInode(inode_buffer, getInodeForFile(filename));
	}
	
	//initialize OFT
	printf("%d\n", oftIndex);
	printf("%s\n", filename);
	printf("%d\n", MAX_FNAME_LEN);
	printf("%d\n", strlen(_oft[oftIndex].filename));
	strncpy(_oft[oftIndex].filename, filename, MAX_FNAME_LEN);
	_oft[oftIndex].taken = 1;
	_oft[oftIndex].openMode = mode;
	_oft[oftIndex].blockSize = _fs->blockSize;
	_oft[oftIndex].inode = getInodeForFile(filename);
	_oft[oftIndex].inodeBuffer = inode_buffer;
	_oft[oftIndex].buffer = makeDataBuffer();
	_oft[oftIndex].writePtr = 0;
	_oft[oftIndex].readPtr = 0;
	_oft[oftIndex].filePtr = 0;
	
	//increase oft counter
	_oftCount++;
	printf("4\n");
	return oftIndex;
}

// Write data to the file. File must be opened in MODE_NORMAL or MODE_CREATE modes. Does nothing
// if file is opened in MODE_READ_ONLY mode.
void writeFile(int fp, void *buffer, unsigned int dataSize, unsigned int dataCount)
{
	if (_oft[fp].openMode == MODE_READ_ONLY) {
        return;
    }
    unsigned long blockNumber;
    unsigned long dataLength = dataCount * dataSize;

    unsigned long fileLength = getFileLength(_oft[fp].filename);
    updateDirectoryFileLength(_oft[fp].filename, fileLength + dataLength);

    for (int i = 0; i < dataLength ; i++) {
        if (_oft[fp].writePtr == _fs->blockSize) {
            blockNumber = returnBlockNumFromInode(_oft[fp].inodeBuffer,
                                                   _oft[fp].filePtr- 1);
            writeBlock(_oft[fp].buffer, blockNumber);

            blockNumber = findFreeBlock();
            if (blockNumber == FS_FULL) {
            	printf("Couldn't find any free block.\n");
            	return;
            }
            setBlockNumInInode(_oft[fp].inodeBuffer,
                               _oft[fp].filePtr, blockNumber);
            markBlockBusy(blockNumber);

            // Clean up the buffer and reset the pointer
            memset(_oft[fp].buffer, 0, _fs->blockSize);
            _oft[fp].writePtr = 0;
        }

        // Copy by character
        unsigned int cptr = _oft[fp].writePtr++;
        memcpy(_oft[fp].buffer + cptr, ((char * ) buffer) + i, sizeof(char));
        _oft[fp].filePtr++;
    }
}

// Flush the file data to the disk. Writes all data buffers, updates directory,
// free list and inode for this file.
void flushFile(int fp)
{
	unsigned long blockNumber = returnBlockNumFromInode(_oft[fp].inodeBuffer, _oft[fp].filePtr);
	writeBlock(_oft[fp].buffer, blockNumber);
	updateFreeList();
	saveInode(_oft[fp].inodeBuffer, _oft[fp].inode);
	updateDirectory();
}

// Read data from the file.
void readFile(int fp, void *buffer, unsigned int dataSize, unsigned int dataCount)
{	
	//current block kept in buffer
	//copy from buffer to memory until desired count or EOF
		//update curr position, return status
	//or end of buffer is reached
		//write buffer to disk(if modified)
		//put the next block into buffer
		//continue reading
	unsigned long numBytesRead = 0;
	unsigned long fileLen = getFileLength(_oft[fp].filename);
	
	while(_oft[fp].filePtr < fileLen && numBytesRead < (dataSize * dataCount)) {
		//if the readPtr reaches the end of the block, load next block
		if(_oft[fp].readPtr >= _fs->blockSize) {
			readBlock(_oft[fp].buffer, returnBlockNumFromInode(_oft[fp].inodeBuffer, _oft[fp].filePtr));
			_oft[fp].readPtr = 0;
		}
		memcpy(((char *)buffer) + numBytesRead, _oft[fp].buffer + _oft[fp].readPtr, sizeof(char));
		_oft[fp].readPtr++;
		_oft[fp].filePtr++;
		numBytesRead++;
	}
	_result = numBytesRead;
}

// Delete the file. Read-only flag (bit 2 of the attr field) in directory listing must not be set. 
// See TDirectory structure.
void delFile(const char *filename)
{	
	if(getattr(filename) & 0b100) {
		return;
	}
	//search directory
	unsigned int fileIndex = findFile(filename);
	if(fileIndex == FS_FILE_NOT_FOUND) {
		return;
	}
	
	//free Inode
	unsigned long *inode_buffer = makeInodeBuffer();
	loadInode(inode_buffer, getInodeForFile(filename));

	//mark every block in inode as free
	for(int i = 0; i < _fs->numInodeEntries; i++) {
		//free data blocks
		markBlockFree(inode_buffer[i]);
	}
	updateFreeList();
	free(inode_buffer);
	
	//free entry
	delDirectoryEntry(filename);
	updateDirectory();
}

// Close a file. Flushes all data buffers, updates inode, directory, etc.
void closeFile(int fp) {
	flushFile(fp);
	free(_oft[fp].buffer);
	free(_oft[fp].inodeBuffer);
	//TODO: oft
}

// Unmount file system.
void closeFS() {
	unmountFS();
	//TODO: free more things
}

