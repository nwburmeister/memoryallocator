#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "mem.h"
int main() {
   assert(Mem_Init(4096) == 0);
   void * ptr[4];

   ptr[0] = Mem_Alloc(800);
   assert(ptr[0] != NULL);

   ptr[1] = Mem_Alloc(800);
   assert(ptr[1] != NULL);

   ptr[2] = Mem_Alloc(800);
   assert(ptr[2] != NULL);

   ptr[3] = Mem_Alloc(800);
   assert(ptr[3] != NULL);

   while (Mem_Alloc(800) != NULL)
     ;

   assert(Mem_Free(ptr[1]) == 0);
   assert(Mem_Free(ptr[2]) == 0);

   ptr[2] = Mem_Alloc(1600);
   assert(ptr[2] != NULL);
   
   printf("HERE");
   // -------------------
   ptr[0] = Mem_Alloc(800);
   printf("HERE2");
   assert(ptr[0] != NULL);
   printf("HERE2");
   ptr[1] = Mem_Alloc(800);
   assert(ptr[1] != NULL);

   ptr[2] = Mem_Alloc(800);
   assert(ptr[2] != NULL);

   ptr[3] = Mem_Alloc(800);
   assert(ptr[3] != NULL);
   printf("HERE2");
   while (Mem_Alloc(800) != NULL)
     ;
   printf("HERE3");
   assert(Mem_Free(ptr[2]) == 0);
   assert(Mem_Free(ptr[1]) == 0);

   ptr[2] = Mem_Alloc(1600);
   assert(ptr[2] != NULL);
   exit(0);
}

