/* A simple test harness for tree-based memory alloction. */

#include "palloc.h"

#ifdef PALLOC_TEST
int main(int argc, char **argv)
{
        void *parent;
        int *data;

        parent = palloc_init("test");
        data = palloc(parent, int);
        pfree(data);

        return 0;
}
#endif
