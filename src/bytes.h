#pragma once
#include "types.h"
#include <stddef.h>

typedef struct Bytes
{
    u8 *data;
    size_t len;
    size_t capacity;
} Bytes;

void Bytes_append(Bytes *self, u8 data);
void Bytes_deinit(Bytes *self);
