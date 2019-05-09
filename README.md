# libmo
libmo是针对游戏服务器的内存管理库，支持多线程，提供malloc和free接口，以及hook系统api的功能
使用方法：

```
mo_hook();

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

mo_restore();
```
