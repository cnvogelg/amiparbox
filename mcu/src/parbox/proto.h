#ifndef PROTO_H

#include "proto_shared.h"

extern void proto_init(void);
extern void proto_handle(void);

// define these in your code
extern void proto_api_action(u08 num);

extern void proto_api_set_rw_reg(u08 reg,u16 val);
extern u16  proto_api_get_rw_reg(u08 reg);
extern u16  proto_api_get_ro_reg(u08 num);

extern u08 *proto_api_read_msg_prepare(u08 chan,u16 *size);
extern void proto_api_read_msg_done(u08 chan);
extern u08 *proto_api_write_msg_prepare(u08 chan,u16 *max_size);
extern void proto_api_write_msg_done(u08 chan,u16 size);

#endif
