#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string>

#define ERROR_EXIT(...)                                                                                                \
    fprintf(stderr, __VA_ARGS__);                                                                                      \
    exit(1)
#define ERROR_RETURN(R, ...)                                                                                           \
    fprintf(stderr, __VA_ARGS__);                                                                                      \
    return R

namespace Nova
{
}
