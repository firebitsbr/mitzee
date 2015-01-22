/*
	see copyright notice in squirrel.h
*/
#include <assert.h>
#include "sqpcheader.h"

#define ALIGN_TO 8
__thread long __memcount = 0;
__thread bool __enable_log = false;

#include <map>
 static std::map<void*, SQUnsignedInteger> __pointers;

void *sq_vm_malloc(SQUnsignedInteger size)
{
    void* p;
    assert(size);
    posix_memalign(&p, ALIGN_TO, size);
    __memcount++;
    if(__enable_log){
        printf("++ %d   %d  %p\n",__memcount, size, p);
        __pointers[(void*)p]=size;
    }
    return p;
//mco    return malloc(size);
}
#if 0
void *sq_vm_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger size)
{ return realloc(p, size); }
#else //mco
void *sq_vm_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger size)
{
    if(0 == p)return sq_vm_malloc(size);
    assert(oldsize);
    if(0 == size){
        printf("realloc to 0 !!!\n");
        __memcount++;
        free (p);
        return 0;
    }
    /*
    void* pn;// = malloc(size);
    posix_memalign(&pn, 8, size);
    memcpy(pn, p, oldsize);
    free(p);
    return pn;
    */
    size = ((size + (ALIGN_TO-1)) / ALIGN_TO) * ALIGN_TO;
    return realloc(p, size);
}
#endif


void sq_vm_free(void *p, SQUnsignedInteger size){
    __memcount--;
    if(__enable_log){
        printf("-- %d  %d   %p\n",__memcount, size, p);
        __pointers.erase((void*)p);
    }
    free(p);
}

void PRINT_REMAIN()
{
    std::map<void*, SQUnsignedInteger>::iterator b =__pointers.begin();
    printf("unfeeed pointers %d \n", __pointers.size());
    size_t ammnt = 0;
    for(;b!= __pointers.end(); b++)
    {
        printf("%p: %d %s \n", b->first, b->second,  (char*)(b->first));
        //free((void*)b->first);
        ammnt+=b->second;
    }
    printf("unfeeed memory %d \n", ammnt);
    __pointers.clear();

}
