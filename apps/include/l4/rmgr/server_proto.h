#ifndef __L4_SERVER_PROTO_H__
#define __L4_SERVER_PROTO_H__

#include <l4/l4.h>

typedef struct {
  word_t param;
  byte_t action;
  byte_t proto;
} l4_proto_struct_t;

typedef union {
  l4_proto_struct_t proto;
  dword_t request;
} l4_proto_t;

#define L4_PROTO_MSG(proto, action, param) \
  (((proto) << 24) | ((action) << 16) | (param))

#endif
