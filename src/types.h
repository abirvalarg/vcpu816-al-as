#pragma once

typedef unsigned char u8;
typedef unsigned short u16;

_Static_assert(sizeof(u8) == 1, "Bad size for u8");
_Static_assert(sizeof(u16) == 2, "Bad size for u16");
