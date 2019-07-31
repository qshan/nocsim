#ifndef NOCVIZ_UTIL_H
#define NOCVIZ_UTIL_H

#include <stdlib.h>
#include <err.h>

#define noctools_malloc(size) \
	__extension__ ({ \
		void* __res = calloc(size, 1); \
		if (__res == NULL) { warn("malloc failed"); } \
		__res;})

#endif
