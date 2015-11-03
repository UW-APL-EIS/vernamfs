#ifndef _ONETIMEPADFS_H
#define _ONETIMEPADFS_H

#include <stdint.h>

typedef struct {
  uint64_t magic;
} OTPHeader;

extern uint64_t MAGIC;

int OTPHeaderInit( OTPHeader* thiz );

#endif
