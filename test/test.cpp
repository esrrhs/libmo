#include "mo.h"
#include <iostream>
#include <stdio.h>
#ifndef WIN32
#include "gperftools/profiler.h"
#endif

int main(int argc, char *argv[])
{
#ifndef WIN32
#ifndef _DEBUG
	ProfilerStart("test.prof");
#endif
#endif
    mo_hook();

#ifndef _DEBUG
    for (int i = 0; i < 100000; i++)
#else
    for (int i = 0; i < 10; i++)
#endif
    {
        void * p = mo_alloc(i);
        size_t s = mo_memsize(p);
        p = mo_realloc(p, 2 * s);
        mo_free(p);
        p = mo_memalign(8, i);
        mo_free(p);
        p = mo_calloc(1, i);
        mo_free(p);
        void * ppp = malloc(32);
        free(ppp);
        int * pp = new int[32];
        delete []pp;
    }

    mo_restore();
    
#ifndef WIN32
#ifndef _DEBUG
	ProfilerStop();
#endif
#endif
		
	std::cout<<"finish"<<std::endl;
	char c;
	std::cin >> c;
	return 0;
}
