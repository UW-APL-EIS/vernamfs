#include "onetimepadfs.h"

uint64_t MAGIC = (uint64_t)0x53464d414e524556;

int OTPHeaderInit( OTPHeader* thiz ) {
  thiz->magic = MAGIC;
  return 0;
}

void OTPHeaderWrite( OTPHeader* thiz, void* addr ) {
  OTPHeader* p = (OTPHeader*)addr;
  p->magic = thiz->magic;
}

// eof
