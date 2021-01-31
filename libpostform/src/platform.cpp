
#include "postform/utils.h"

struct PostformPlaformDescription {
    uint32_t char_size = sizeof(char);
    uint32_t short_size = sizeof(short);
    uint32_t int_size = sizeof(int);
    uint32_t long_int_size = sizeof(long int);
    uint32_t long_long_int_size = sizeof(long long int);
};

CLINKAGE __attribute__((section(".postform_platform_descriptors")))
const PostformPlaformDescription _postform_platform_description;
