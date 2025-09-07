# **Assignment 1 - Storage Manager**

The goal of this assignment is to implement a simple storage manager-a module that is capable of reading blocks from a file on disk into memory and writing blocks from memory to a file on disk. The storage manager deals with pages(blocks) of fixed size(PAGESIZE). In addition to reading and writing pages from a file, it provides methods for creating, opening, and closing files. The storage manager has to maintain several types of information for an open file: The number of total pages in the file, the current page position (for reading and writing), the file name, and a POSIX file descriptor or FILE pointer.

-----------------------------------------------------------------------------------------------------------------

### Static (Private) Helper Methods

### addOpenFile:

Appends a new node (fileName, fd) to the internal linked list of open files. Helps keep track of currently opened files.

### removeOpenFile: 

Removes a node (by file descriptor) from the linked list of open files. Used when closing or destroying a file.

### getFdByName:

Searches the linked list of open files by file name and returns the file descriptor if found, else -1.

### page_offset:

Computes the byte offset of a given page index: pageIndex * PAGE_SIZE. Used to position the file pointer for reading/writing.

### get_fd:

Retrieves the underlying OS file descriptor from an SM_FileHandle. Returns -1 if invalid.

### updatePageCount:

Uses fstat to get the file size and updates fHandle->totalNumPages to reflect the number of logical pages.

### write_zero_page_fd:

Writes a zero-filled page of size PAGE_SIZE to the file at the current position. Used in createPageFile, appendEmptyBlock, and ensureCapacity.

-----------------------------------------------------------------------------------------------
# **Page Related Methods**

 ### initStorageManager:

This method is called to initialize the storage manager.

### createPageFile:

-This method creates the given filename of default page size .

-We use fopen() C function to create a file. We use 'w+' mode which creates a new file and opens it for reading and writing.

- Return RC_FILE_NOT_FOUND if file was not created and RC_OK if it is created.

### openPageFile:

-We use the fopen() C function to open the file and 'r' mode to open the file in read only mode.

-We initialise struct FileHandle's curPagePos and totalPageNum.

-We use the linux fstat() C function (gives information about the file). We retrieve the size of file in bats using fstat() function.

-We return RC_FILE_NOT_FOUND if file cannot open and RC_OK if file open.

### closePageFile:

This closes file. The parameter that is used is fhandle.

### destroyPageFile:

This destroys the file using remove() also a check is applied if a file is tried to open after deletion, it gives an error.

-------------------------------------------------------------------------------------------

# **Read Related Methods**

The read related functions are used to read blocks of data from the page file into the disk .



### readBlock:

This method reads the pageNum'th block from a file & stores its content in the memory pointed to by the memPage page handle. If the file has less than ‘pageNum’ pages, the method return RC_READ_NON_EXISTING_PAGE. ‘lseek()’ is used to set the offset to the pointer. (PAGE_SIZE * pageNum)

### getBlockPos:

This method returns the current page position.

### readFirstBlock:

We call the readBlock(...) function by providing the pageNum argument as 0.

### readPreviousBlock:

This method reads the previous page relative to the curPagePos of the file. In lseek() offset is set to PAGE_SIZE.

### readCurrentBlock:

We call the readBlock(...) function by providing the pageNum argument as (current page position).

### readNextBlock:

 This method reads the next page relative to the curPagePos of the file. In lseek() offset is set to PAGE_SIZE.

### readLastBlock:

This method is used to read the last block.

-----------------------------------------------------------------------------------------------------------------

# **Write Related Methods**

### writeBlock:

We check whether the page number is valid or not. Page number should be greater than 0 and less than total number of pages.

### writeCurrentBlock:

This method writes the current page to disk(memory) . In lseek(), offset is set to 0 and whence is set to SEEK_CUR.

### appendEmptyBlock:

This method increases the number of pages in the file by adding one page at the end of the file.

### ensureCapacity:

- Check number of pages required is greater than the total number of pages .

- Calculate number of pages required and add empty blocks.

- Empty blocks are added using the appendEmptyBlock function

-------------------------------------------------------------------------------------------

### How to Run (FOR WINDOWS) :
-  Pre-requsisite : MinGW should be installed in the system.

Step 1 : Open the Command Prompt/Terminal.

Step 2 : Navigate to the downloaded directory.

Step 3 : Type command 'mingw32-make' and enter(Files are compiled and ready to be executed).

Step 4 : Type command 'test_assign1.exe'.

---------------------------------------------------------------------------------------------
### How to Run (FOR macOS) :
-  Pre-requsisite : MinGW should be installed in the system.

Step 1 : Open the Command Prompt/Terminal.

Step 2 : Navigate to the downloaded directory in the terminal.

Step 3 : Type command 'make' and enter(Files are compiled and ready to be executed).

Step 4 : Type command './test_assign1'.

