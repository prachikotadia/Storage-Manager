#include "storage_mgr.h"
#include "dberror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initStorageManager(void) {
    printf("Storage Manager initialized.\n");
}
RC createPageFile(char *fileName) {
    FILE *file = fopen(fileName, "w+");
    if (file == NULL) {
        return RC_FILE_NOT_FOUND; 
    }

    SM_PageHandle emptyPage = calloc(PAGE_SIZE, 1);
    if (emptyPage == NULL) {
        fclose(file);
        return RC_WRITE_FAILED;
    }

    size_t writeSize = fwrite(emptyPage, sizeof(char), PAGE_SIZE, file);
    free(emptyPage);
    fclose(file);

    if (writeSize < PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }
    return RC_OK;
}

RC openPageFile(char *fileName, SM_FileHandle *fileHandle) {
    fileHandle->mgmtInfo = fopen(fileName, "r+");
    if (fileHandle->mgmtInfo == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    fileHandle->fileName = strdup(fileName);
    fileHandle->curPagePos = 0;
    
    fseek(fileHandle->mgmtInfo, 0, SEEK_END);
    long fileSize = ftell(fileHandle->mgmtInfo);
    fileHandle->totalNumPages = fileSize / PAGE_SIZE;
    fseek(fileHandle->mgmtInfo, 0, SEEK_SET);

    return RC_OK;
}


RC closePageFile(SM_FileHandle *fileHandle) {
    if (fileHandle == NULL || fileHandle->mgmtInfo == NULL) {
        printf("Error: File handle or file pointer is NULL\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }

    if (fclose(fileHandle->mgmtInfo) == 0) {
        printf("File closed successfully.\n");
        fileHandle->mgmtInfo = NULL; 
        return RC_OK;
    } else {
        perror("Error closing file");
        return RC_FILE_NOT_FOUND;
    }
}


RC destroyPageFile(char *fileName) {
    if (remove(fileName) != 0) {
        return RC_FILE_NOT_FOUND; 
    }
    return RC_OK;
}

/* Read a block from a page file */
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if the file handle and memory page are valid

    if (fHandle == NULL || memPage == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Check if the page number is within range
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Seek to the page position
    fseek(fHandle->mgmtInfo, pageNum * PAGE_SIZE, SEEK_SET);
    // Read the page into the memory page buffer
    size_t bytesRead = fread(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
    // Check if the read was successful
    
    if (bytesRead < PAGE_SIZE) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Update the file handle's current page position
    fHandle->curPagePos = pageNum;

    return RC_OK;
}

/* Get the current block position */
int getBlockPos(SM_FileHandle *fHandle) {
    return fHandle->curPagePos;
}

/* Read the first block in a page file */
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(0, fHandle, memPage);
}

/* Read the previous block in a page file */
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int currentPos = getBlockPos(fHandle);
    if (currentPos <= 0) {
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(currentPos - 1, fHandle, memPage);
}

/* Read the current block in a page file */
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int currentPos = getBlockPos(fHandle);
    return readBlock(currentPos, fHandle, memPage);
}

/* Read the next block in a page file */
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int currentPos = getBlockPos(fHandle);
    if (currentPos >= fHandle->totalNumPages - 1) {
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(currentPos + 1, fHandle, memPage);
}

/* Read the last block in a page file */
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}

/* Write a block to a page file */
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if the page number is within range
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        return RC_WRITE_FAILED;
    }

    // Seek to the page position
    fseek(fHandle->mgmtInfo, pageNum * PAGE_SIZE, SEEK_SET);
    // Write the memory page buffer to the file
    size_t writtenbytes = fwrite(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);

    // Check if the write was successful
    if (writtenbytes < PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }

    return RC_OK;
}

/* Write the current block to a page file */
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

/* Append an empty block to a page file */
RC appendEmptyBlock(SM_FileHandle *fHandle) {
    // Create an empty memory page buffer
    SM_PageHandle emptyPage = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
    if (emptyPage == NULL) {
        return RC_WRITE_FAILED;
    }

    // Seek to the end of the file
    fseek(fHandle->mgmtInfo, 0, SEEK_END);
    // Write the empty memory page buffer to the file
    size_t writtenbytes = fwrite(emptyPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
    free(emptyPage);

    // Check if the write was successful
    if (writtenbytes < PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }

    // Increase the total number of pages
    fHandle->totalNumPages++;
    return RC_OK;
}

RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    int currentNumPages = fHandle->totalNumPages;
    if (numberOfPages > currentNumPages) {
        // Calculate the number of pages to append
        int PagesToAppend = numberOfPages - currentNumPages;
        // Append empty blocks to the file until it reaches the desired capacity
        for (int i = 0; i < PagesToAppend; i++) {
            RC rc = appendEmptyBlock(fHandle);
            if (rc != RC_OK) {
                return rc;
            }
        }
    }
    return RC_OK;
}