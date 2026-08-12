#pragma once
#include <stdint.h>
#include <sys/types.h>

#define ANDROID_DLEXT_RESERVED_ADDRESS 0x1
#define ANDROID_DLEXT_USE_LIBRARY_FD   0x10

typedef struct {
    uint64_t flags;
    void*    reserved_addr;
    size_t   reserved_size;
    int      relro_fd;
    int      library_fd;
    off64_t  library_fd_offset;
    void*    library_namespace;
} android_dlextinfo;
