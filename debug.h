#ifndef __DEBUG_HEADER__
#define __DEBUG_HEADER__
#include <stdio.h>
#include <stdlib.h>

#define DEBUG

#ifdef DEBUG
#       define _DEBUG_MSG(e,...) fprintf(stderr, "[%s:%d] " e "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#       define _DEBUG_MSG(e,...) do {} while(0);
#endif

#endif

