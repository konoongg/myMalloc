#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "myMalloc.h"

enum EStatus{
    BORROW,
    FREE
} typedef EStatus;

struct Tmemmory{
    size_t Size;
    EStatus Status;
    struct Tmemmory* Left;
    struct Tmemmory* Right;
    void* Start;
} typedef Tmemmory ;

Tmemmory* curMemmory = NULL;
int countBlock;

void ClearMem(void* mem, int size){
    memset(mem, 0, size);
}

void CreateNewBlock(void* start, size_t size){
    Tmemmory* newBlock  = (Tmemmory*)malloc(sizeof(Tmemmory));
    newBlock->Right = curMemmory->Right;
    newBlock->Right->Left = newBlock;
    newBlock->Status = FREE;
    newBlock->Left = curMemmory;
    newBlock->Start = start;
    newBlock->Size = size;
    curMemmory->Right = newBlock;
}

void BorrowBlock(size_t size){
    curMemmory->Size = size;
    curMemmory->Status = BORROW;
}

void* MyMalloc(size_t size){
    if(curMemmory == NULL){
        countBlock = 1;
        const int pageSize = getpagesize();
        size_t mmapSize = (size / pageSize + 1) * pageSize;
        void* newRegion = mmap(NULL, mmapSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (newRegion == MAP_FAILED) {
            return NULL;
        }
        curMemmory = (Tmemmory*)malloc(sizeof(Tmemmory));
        curMemmory->Right = curMemmory;
        curMemmory->Left = curMemmory;
        curMemmory->Size = mmapSize;
        curMemmory->Status = FREE;
        curMemmory->Start = newRegion;
    }
    int curNumBlock = 0;
    void* mem = NULL;
    while(curNumBlock < countBlock ){
        if(curMemmory->Status == BORROW){
            curMemmory = curMemmory->Right;
            curNumBlock++;
            continue;
        }
        if(curMemmory->Size > size){
            size_t freeSize = curMemmory->Size - size;
            BorrowBlock(size);
            CreateNewBlock(curMemmory->Start + size, freeSize);
            countBlock++;
            mem = curMemmory->Start;
            curMemmory = curMemmory->Right;
            break;
        }
        else if(curMemmory->Size == size){
            BorrowBlock(size);
            mem = curMemmory->Start;
            break;
        }
        else{
            curMemmory = curMemmory->Right;
            curNumBlock++;
        }
    }
    if(mem == NULL){
        const int pageSize = getpagesize();
        size_t mmapSize = (size / pageSize + 1) * pageSize;
        void* newRegion = mmap(NULL, mmapSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        CreateNewBlock(newRegion, mmapSize);
        countBlock++;
        if (newRegion == MAP_FAILED) {
            return NULL;
        }
        curMemmory = curMemmory->Right;
        if(curMemmory->Size > size){
            size_t freeSize = curMemmory->Size - size;
            BorrowBlock(size);
            CreateNewBlock(curMemmory->Start + size, freeSize);
            countBlock++;
            mem = curMemmory->Start;
            curMemmory = curMemmory->Right;
        }
        else if(curMemmory->Size == size){
            BorrowBlock(size);
            mem = curMemmory->Start;
        }
    }
    ClearMem(mem, size);
    return mem;
}

void FreeBlock(){
    Tmemmory* curLeft = curMemmory->Left;
    Tmemmory* curRight = curMemmory->Right;
    if(curLeft->Status == FREE && curRight->Status == FREE && curRight->Start != curLeft->Start){
        curLeft->Size += curMemmory->Size + curRight->Size;
        curLeft->Right = curRight->Right;
        curMemmory->Right->Right->Left = curMemmory->Left;
        countBlock -= 2;
        free(curMemmory);
        free(curRight);
        curMemmory = curLeft;
    }
    else if(curRight->Status == FREE){
        curRight->Size += curMemmory->Size;
        curRight->Start = curMemmory->Start;
        curRight->Left = curMemmory->Left;
        curMemmory->Left->Right = curMemmory->Right;
        countBlock--;
        free(curMemmory);
        curMemmory = curRight;
    }
    else if(curLeft->Status == FREE){
        curLeft->Size += curMemmory->Size;
        curLeft->Right = curMemmory->Right;
        curMemmory->Right->Left = curMemmory->Left;
        countBlock--;
        free(curMemmory);
        curMemmory = curLeft;
    }
    else{
        CreateNewBlock(curMemmory->Start, curMemmory->Size);
        curMemmory = curMemmory->Right;
    }
}

void MyFree(void* mem){
    if(mem == NULL){
        return;
    }
    char isFree = 0;
    int curNumBlock = 0;
    while(curNumBlock < countBlock ){
        if(curMemmory->Start == mem){
            FreeBlock();
            isFree = 1;
            break;
        }
        else{
            curMemmory = curMemmory->Right;
            curNumBlock++;
        }
    }
    if (isFree == 0){
        fprintf(stderr, "%p not alloc \n", mem);
        exit(EXIT_FAILURE);
    }
}
