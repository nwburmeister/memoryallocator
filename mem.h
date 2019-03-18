#ifndef __mem_h__
#define __mem_h__

int Mem_Init(int sizeOfRegion);
void* Mem_Alloc(int size);
int Mem_Free(void *ptr);
void Mem_Dump();

void* malloc(size_t size) {
    return NULL;
}

#endif // __mem_h__


