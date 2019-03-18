////////////////////////////////////////////////////////////////////////////////
// Main File:        (N/A)
// This File:        (mem.c)
// Other Files:      (name of all other files if any)
// Semester:         CS 354 Spring 2018
//
// Author:           (Nik Burmeister)
// Email:            (nburmeister@wisc.edu)
// CS Login:         (nikolas@cs.wisc.edu)
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          Identify persons by name, relationship to you, and email.
//                   Describe in detail the the ideas and help they provided.
//
// Online sources:   avoid web searches to solve your problems, but if you do
//                   search, be sure to include Web URLs and description of 
//                   of any information you find.
//////////////////////////// 80 columns wide ///////////////////////////////////
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "mem.h"

/*
 * This structure serves as the header for each allocated and free block
 * It also serves as the footer for each free block
 * The blocks are ordered in the increasing order of addresses 
 */
typedef struct blk_hdr {                         
        int size_status;
  
    /*
    * Size of the block is always a multiple of 8
    * => last two bits are always zero - can be used to store other information
    *
    * LSB -> Least Significant Bit (Last Bit)
    * SLB -> Second Last Bit 
    * LSB = 0 => free block
    * LSB = 1 => allocated/busy block
    * SLB = 0 => previous block is free
    * SLB = 1 => previous block is allocated/busy
    * 
    * When used as the footer the last two bits should be zero
    */

    /*
    * Examples:
    * 
    * For a busy block with a payload of 20 bytes (i.e. 20 bytes data + an additional 4 bytes for header)
    * Header:
    * If the previous block is allocated, size_status should be set to 27
    * If the previous block is free, size_status should be set to 25
    * 
    * For a free block of size 24 bytes (including 4 bytes for header + 4 bytes for footer)
    * Header:
    * If the previous block is allocated, size_status should be set to 26
    * If the previous block is free, size_status should be set to 24
    * Footer:
    * size_status should be 24
    * 
    */
} blk_hdr;

/* Global variable - This will always point to the first block
 * i.e. the block with the lowest address */
blk_hdr *first_blk = NULL;

/*
 * Note: 
 *  The end of the available memory can be determined using end_mark
 *  The size_status of end_mark has a value of 1
 *
 */

/* 
 * Function for allocating 'size' bytes
 * Returns address of allocated block on success 
 * Returns NULL on failure 
 * Here is what this function should accomplish 
 * - Check for sanity of size - Return NULL when appropriate 
 * - Round up size to a multiple of 8 
 * - Traverse the list of blocks and allocate the best free block which can accommodate the requested size 
 * - Also, when allocating a block - split it into two blocks
 * Tips: Be careful with pointer arithmetic 
 */
void* Mem_Alloc(int size) {                      
    // Your code goes in here 
    
    int padding = (size % 4);  // used to find next multiple of a word
    if ( padding > 0 ) {
        size = size + 4 - padding;  //  round to multiple of a word
    }
    size += sizeof(blk_hdr);  // assign space for header 
    while ( (size % 8) != 0 ) {
        size += 4;  //  if not a mult of 8, add 4
    }

    blk_hdr *current = first_blk;  //  current block in heap
    blk_hdr *best_block = NULL;  //  used to track best block
    int best_block_size = -1;  //  stores the best block size
    int block_size = -1;  //  current block size 
    int best_diff = -1;  //  tracks best difference
    int status = -1;   //  current block status
    
    while ( current->size_status != 1 ) {
        status = current->size_status % 8;
        block_size = current->size_status - status;
        if ((status == 2) && (block_size >= size)) {
            int tmp_diff = block_size - size;
            if ((best_diff < 0)  || (tmp_diff < best_diff)) {
               best_diff = tmp_diff;
               best_block = current;
               best_block_size = block_size;
            }
        }
        current = (blk_hdr*)((void*)current + block_size);
    }

    if ( best_block == NULL || best_block_size < 0 ) {
        return NULL;
    }

    //  inform block in front of you
    blk_hdr *next_block = (blk_hdr*) ((void*)best_block + best_block_size);
    if ( next_block->size_status != 1 ) {
        next_block->size_status += 2;   //  indicate that p block is alloc'd  
    } 
    //  create new header
    blk_hdr *new_header = (blk_hdr*) ((void*)best_block + 
                                             best_block_size - size);
    
    //  set status of new block 
    int indicator = 1;  //  for the new blocks status
    if ( first_blk == new_header || best_diff == 0 ) {
        indicator = 3;
    }
    new_header->size_status = size + indicator;
    
    //  get rid of old footer
    blk_hdr *old_footer = (blk_hdr*) ((void*)best_block + 
                                             best_block_size - sizeof(blk_hdr));
    old_footer->size_status = 0;
    
    //  set footer of block you pushed aside
    blk_hdr *p_block_footer = (blk_hdr*) ((void*)best_block + best_block_size
                                                 - size - sizeof(blk_hdr));
    p_block_footer->size_status = best_block_size - size;
    if ( best_diff != 0 ) {
        //  when there isn't an exact match, reset the size of the p_block
        blk_hdr *p_block = (blk_hdr*) ((void*)best_block);
        p_block->size_status = best_block_size - size + 2;    
    }
    
    //  return the header + 4       
    return (blk_hdr*)((void*)new_header + sizeof(blk_hdr));
}

/* 
 * Function for freeing up a previously allocated block 
 * Argument - ptr: Address of the block to be freed up 
 * Returns 0 on success 
 * Returns -1 on failure 
 * Here is what this function should accomplish 
 * - Return -1 if ptr is NULL
 * - Return -1 if ptr is not 8 byte aligned or if the block is already freed
 * - Mark the block as free 
 * - Coalesce if one or both of the immediate neighbours are free 
 */
int Mem_Free(void *ptr) {                        
    //  Your code goes in here

    //  get to the header
    blk_hdr *ptr1 = ((void *)ptr - sizeof(blk_hdr));

    //  check if ptr is null or not div by 8    
    if ( ptr == NULL || ((int)ptr % 8 != 0 ) ) {
        return -1;
    }
    
    //  get the size of the ptr
    int ptr_size = ptr1->size_status;
    //  get the status
    int ptr_status = ptr_size % 8; 
    //  adjust for actual size 
    ptr_size -= ptr_status;
     
    if ( ptr_status == 2 ) {
        return -1;  //  block already free
    }
    
    //  set to free block
    ptr1->size_status -= 1;
    
    //  check left block
    int left_block_size = 0;
    blk_hdr *left_block = ptr1;  //  set to current header  
    if ( ptr_status == 1 ) {
        //  get the footer
        blk_hdr *left_footer = (blk_hdr*)((void*)ptr1 - sizeof(blk_hdr));
        //  store the block size
        left_block_size = left_footer->size_status;
        //  set footer to 0
        left_footer->size_status = 0;
        //  update left block position
        left_block = (blk_hdr*)((void*)ptr1 - left_block_size);
    }
    
    //  check right block
    blk_hdr *right_block = (blk_hdr*)((void*)ptr1 + ptr_size);
    int right_block_size = right_block->size_status;  //  get size of r_block
    
    if ( right_block_size % 8 == 2 ) {
        //  coalesce with right block
        right_block_size -= 2;  //  get actual block size
        right_block = NULL;  //  set to null 
    } else if ( right_block_size % 8 == 3 ) {
        //  right block is used, change p bit
        right_block->size_status -= 2;       
        right_block_size = 0;
    } else if ( right_block_size == 1 ) {
        right_block_size = 0;
    }

    //  new header always starts at left block
    blk_hdr *new_hdr = (blk_hdr*)((void*)left_block);  
    //  +2 to indicate block before it is allocated
    new_hdr->size_status = left_block_size + ptr_size 
                                           + right_block_size + 2;
    //  need a word for the footer 
    blk_hdr *footer = (blk_hdr*)((void*)left_block + 
                                       (left_block_size + ptr_size + 
                                        right_block_size - 
                                        sizeof(blk_hdr)));     
    footer->size_status = left_block_size + ptr_size + right_block_size;
    
    return 0;    
}

/*
 * Function used to initialize the memory allocator
 * Not intended to be called more than once by a program
 * Argument - sizeOfRegion: Specifies the size of the chunk which needs to be allocated
 * Returns 0 on success and -1 on failure 
 */
int Mem_Init(int sizeOfRegion) {                         
    int pagesize;
    int padsize;
    int fd;
    int alloc_size;
    void* space_ptr;
    blk_hdr* end_mark;
    static int allocated_once = 0;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: Mem_Init has allocated space during a previous call\n");
        return -1;
    }
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    alloc_size = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    space_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, 
                    fd, 0);
    if (MAP_FAILED == space_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
     allocated_once = 1;

    // for double word alignement and end mark
    alloc_size -= 8;

    // To begin with there is only one big free block
    // initialize heap so that first block meets 
    // double word alignement requirement
    first_blk = (blk_hdr*) space_ptr + 1;
    end_mark = (blk_hdr*)((void*)first_blk + alloc_size);
  
    // Setting up the header
    first_blk->size_status = alloc_size;

    // Marking the previous block as busy
    first_blk->size_status += 2;

    // Setting up the end mark and marking it as busy
    end_mark->size_status = 1;

    // Setting up the footer
    blk_hdr *footer = (blk_hdr*) ((char*)first_blk + alloc_size - 4);
    footer->size_status = alloc_size;
  
    return 0;
}

/* 
 * Function to be used for debugging 
 * Prints out a list of all the blocks along with the following information i
 * for each block 
 * No.      : serial number of the block 
 * Status   : free/busy 
 * Prev     : status of previous block free/busy
 * t_Begin  : address of the first byte in the block (this is where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block (as stored in the block header) (including the header/footer)
 */ 
void Mem_Dump() {                        
    int counter;
    char status[5];
    char p_status[5];
    char *t_begin = NULL;
    char *t_end = NULL;
    int t_size;

    blk_hdr *current = first_blk;
    counter = 1;

    int busy_size = 0;
    int free_size = 0;
    int is_busy = -1;

    fprintf(stdout, "************************************Block list***\
                    ********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, "-------------------------------------------------\
                    --------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
        int footer_val = -1;
        blk_hdr *footer_value = NULL;
        if (t_size & 1) {
            // LSB = 1 => busy block
            strcpy(status, "Busy");
            is_busy = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "Free");
            is_busy = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "Busy");
            t_size = t_size - 2;
            footer_value = (blk_hdr*)((void*)current + t_size - 4);
            footer_val = footer_value->size_status;
        } else {
            strcpy(p_status, "Free");
        }

        if (is_busy) 
            busy_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\t%i\t0x%08lx\n", 
        counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size, 
                   footer_val, (unsigned long int)footer_value);
    
        current = (blk_hdr*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, "---------------------------------------------------\
                    ------------------------------\n");
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fprintf(stdout, "Total busy size = %d\n", busy_size);
    fprintf(stdout, "Total free size = %d\n", free_size);
    fprintf(stdout, "Total size = %d\n", busy_size + free_size);
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fflush(stdout);

    return;
}
