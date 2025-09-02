#include <fcntl.h>    
#include <unistd.h>     
#include <sys/stat.h>  
#include <stdlib.h>    
#include <string.h>     
#include "storage_mgr.h"
#include "dberror.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

// // Struct used to store the fHandle->mgmtInfo; holds the OS file descriptor (int)
typedef struct SM_InternalFD {
    int fd;
} SM_InternalFD;
// Struct to get the Linked list for all the open files
typedef struct OpenNode {
    char *name;
    int   fd;
    struct OpenNode *next;
} OpenNode;

static OpenNode *g_open_files = NULL;

static void reg_add(const char *name, int fd) {//Function to append the Linked list for opened files
    OpenNode *n = (OpenNode*)malloc(sizeof(OpenNode));
    if (!n) return;
    n->name = strdup(name ? name : "");
    n->fd = fd;
    n->next = g_open_files;
    g_open_files = n;
}

static void reg_remove_fd(int fd) {//If any file is closed, remove it from the opened files linked list
    OpenNode **pp = &g_open_files;
    while (*pp) {
        if ((*pp)->fd == fd) {
            OpenNode *del = *pp;
            *pp = del->next;
            free(del->name);
            free(del);
            return;
        }
        pp = &((*pp)->next);
    }
}

static int reg_find_fd_by_name(const char *name) {//Function to find the file in the openNode Struct
    for (OpenNode *p = g_open_files; p; p = p->next) {
        if (strcmp(p->name, name) == 0) return p->fd;
    }
    return -1;
}

/* ---------------- Helpers ---------------- */

static inline off_t page_offset(int pageIndex) {
    return (off_t)pageIndex * (off_t)PAGE_SIZE;
}

static inline int get_fd(const SM_FileHandle *fh) {
    return (fh && fh->mgmtInfo) ? ((const SM_InternalFD*)fh->mgmtInfo)->fd : -1;
}

static RC refresh_page_count(int fd, SM_FileHandle *fh) {
    struct stat st;
    if (fstat(fd, &st) != 0) return RC_FILE_NOT_FOUND;
    // Round up to full pages (robust to odd file sizes)
    off_t sz = st.st_size;
    fh->totalNumPages = (int)((sz + PAGE_SIZE - 1) / PAGE_SIZE);
    return RC_OK;
}

static RC write_zero_page_fd(int fd) {
    void *zeros = calloc(1, PAGE_SIZE);
    if (!zeros) return RC_WRITE_FAILED;
    ssize_t w = write(fd, zeros, PAGE_SIZE);
    free(zeros);
    return (w == PAGE_SIZE) ? RC_OK : RC_WRITE_FAILED;
}

/* ---------------- API ---------------- */

void initStorageManager(void) {
    printf("Storage Manager has been initialized.");
}

RC createPageFile(char *fileName) {
    int fd = open(fileName, O_CREAT | O_TRUNC | O_RDWR
#ifdef _WIN32
        | O_BINARY
#endif
        , 0644);
    if (fd < 0) return RC_WRITE_FAILED;
    RC rc = write_zero_page_fd(fd);
    close(fd);
    return rc;
}

RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    if (!fHandle || !fileName) return RC_FILE_HANDLE_NOT_INIT;

    int fd = open(fileName, O_RDWR
#ifdef _WIN32
        | O_BINARY
#endif
    );
    if (fd < 0) return RC_FILE_NOT_FOUND;

    SM_InternalFD *box = (SM_InternalFD*)malloc(sizeof(SM_InternalFD));
    if (!box) { close(fd); return RC_FILE_HANDLE_NOT_INIT; }
    box->fd = fd;

    fHandle->fileName   = fileName;
    fHandle->mgmtInfo   = box;
    fHandle->curPagePos = 0;

    RC rc = refresh_page_count(fd, fHandle);
    if (rc != RC_OK) { close(fd); free(box); fHandle->mgmtInfo = NULL; return rc; }
    if (fHandle->totalNumPages == 0) fHandle->totalNumPages = 1; // belt & suspenders

    reg_add(fileName, fd);
    return RC_OK;
}

RC closePageFile(SM_FileHandle *fHandle) {
    if (!fHandle || !fHandle->mgmtInfo) return RC_FILE_HANDLE_NOT_INIT;
    int fd = get_fd(fHandle);
    if (fd >= 0) {
        close(fd);
        reg_remove_fd(fd);
    }
    free(fHandle->mgmtInfo);
    fHandle->mgmtInfo = NULL;
    return RC_OK;
}

RC destroyPageFile(char *fileName) {
    if (!fileName) return RC_FILE_NOT_FOUND;

    // If open (Windows), close our handle first
    int fd_open = reg_find_fd_by_name(fileName);
    if (fd_open >= 0) {
        close(fd_open);
        reg_remove_fd(fd_open);
    }
    return (remove(fileName) == 0) ? RC_OK : RC_FILE_NOT_FOUND;
}

/* ---------------- Reading ---------------- */

RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (!fHandle || !fHandle->mgmtInfo || !memPage) return RC_FILE_HANDLE_NOT_INIT;
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) return RC_READ_NON_EXISTING_PAGE;

    int fd = get_fd(fHandle);
    if (lseek(fd, page_offset(pageNum), SEEK_SET) == (off_t)-1) return RC_READ_NON_EXISTING_PAGE;

    ssize_t r = read(fd, memPage, PAGE_SIZE);
    if (r != PAGE_SIZE) return RC_READ_NON_EXISTING_PAGE;

    fHandle->curPagePos = pageNum;
    return RC_OK;
}

int getBlockPos(SM_FileHandle *fHandle) {
    return fHandle ? fHandle->curPagePos : -1;
}

RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(0, fHandle, memPage);
}

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->curPagePos, fHandle, memPage);
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->curPagePos + 1, fHandle, memPage);
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}

/* ---------------- Writing ---------------- */

RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (!fHandle || !fHandle->mgmtInfo || !memPage) return RC_FILE_HANDLE_NOT_INIT;
    if (pageNum < 0) return RC_WRITE_FAILED;

    int fd = get_fd(fHandle);

    // If writing beyond current capacity, grow first
    if (fHandle->totalNumPages <= pageNum) {
        RC rc = ensureCapacity(pageNum + 1, fHandle);
        if (rc != RC_OK) return rc;
    }

    if (lseek(fd, page_offset(pageNum), SEEK_SET) == (off_t)-1) return RC_WRITE_FAILED;

    ssize_t w = write(fd, memPage, PAGE_SIZE);
    if (w != PAGE_SIZE) return RC_WRITE_FAILED;

    fHandle->curPagePos = pageNum;
    (void)refresh_page_count(fd, fHandle);
    if (fHandle->totalNumPages <= pageNum)
        fHandle->totalNumPages = pageNum + 1;

    return RC_OK;
}

RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

RC appendEmptyBlock(SM_FileHandle *fHandle) {
    if (!fHandle || !fHandle->mgmtInfo) return RC_FILE_HANDLE_NOT_INIT;
    int fd = get_fd(fHandle);
    if (lseek(fd, 0, SEEK_END) == (off_t)-1) return RC_WRITE_FAILED;
    RC rc = write_zero_page_fd(fd);
    if (rc != RC_OK) return rc;
    return refresh_page_count(fd, fHandle);
}

RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    if (!fHandle || !fHandle->mgmtInfo) return RC_FILE_HANDLE_NOT_INIT;
    int fd = get_fd(fHandle);
    RC rc = refresh_page_count(fd, fHandle);
    if (rc != RC_OK) return rc;

    while (fHandle->totalNumPages < numberOfPages) {
        rc = appendEmptyBlock(fHandle);
        if (rc != RC_OK) return rc;
    }
    return RC_OK;
}
