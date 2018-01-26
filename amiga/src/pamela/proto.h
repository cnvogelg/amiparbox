#ifndef PROTO_H
#define PROTO_H

#include "pario.h"
#include "timer.h"
#include "proto_shared.h"
#include "proto_iov.h"

#define PROTO_RET_OK                0
#define PROTO_RET_RAK_INVALID       1
#define PROTO_RET_TIMEOUT           2
#define PROTO_RET_SLAVE_ERROR       3
#define PROTO_RET_MSG_TOO_LARGE     4
#define PROTO_RET_INVALID_CHANNEL   15
#define PROTO_RET_INVALID_FUNCTION  14
#define PROTO_RET_INVALID_ACTION    13

#define PROTO_RET_MASK              0x0f
#define PROTO_RET_STATUS_MASK       0xf0

#define PROTO_KNOK_NOT_FOUND        0
#define PROTO_KNOK_FOUND            1

struct proto_handle;
typedef struct proto_handle proto_handle_t;

extern proto_handle_t *proto_init(struct pario_port *port, struct timer_handle *th, struct Library *SysBase);
extern void proto_exit(proto_handle_t *ph);

extern UBYTE proto_get_status(proto_handle_t *ph);

extern int proto_wait_init(proto_handle_t *ph);
extern int proto_reset(proto_handle_t *ph, int exit_knok);

extern int proto_action(proto_handle_t *ph, UBYTE num);
extern int proto_action_bench(proto_handle_t *ph, UBYTE num, ULONG deltas[2]);

extern int proto_function_read(proto_handle_t *ph, UBYTE num, UWORD *data);
extern int proto_function_write(proto_handle_t *ph, UBYTE num, UWORD data);

extern int proto_function_read_long(proto_handle_t *ph, UBYTE num, ULONG *data);
extern int proto_function_write_long(proto_handle_t *ph, UBYTE num, ULONG data);

extern int proto_msg_write(proto_handle_t *ph, UBYTE chn, proto_iov_t *msgiov);
extern int proto_msg_read(proto_handle_t *ph, UBYTE chn, proto_iov_t *msgiov);

extern int proto_msg_write_single(proto_handle_t *ph, UBYTE chn, UBYTE *buf, UWORD num_words);
extern int proto_msg_read_single(proto_handle_t *ph, UBYTE chn, UBYTE *buf, UWORD *max_words);

extern const char *proto_perror(int res);

#endif
