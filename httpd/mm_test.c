/* A simple test harness for memory alloction. */

#include "mm_alloc.h"

#ifdef MM_TEST
int main(int argc, char **argv)
{
    void *data;

    data = mm_malloc(4);
    free(data);

    return 0;
}
#endif
