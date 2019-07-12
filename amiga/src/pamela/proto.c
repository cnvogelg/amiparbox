#define __NOLIBBASE__
#include <proto/exec.h>

#include "autoconf.h"
#include "compiler.h"
#include "debug.h"

#include "proto.h"
#include "proto_low.h"

/* proto signals */
#define clk_mask    sel_mask
#define rak_mask    pout_mask
#define cflg_mask   busy_mask

#define DDR_DATA_OUT  0xff
#define DDR_DATA_IN   0x00

struct proto_handle {
    struct pario_port   *port;
    struct timer_handle *timer;
    ULONG                timeout_s;
    ULONG                timeout_ms;
    struct Library      *sys_base;
};

proto_handle_t *proto_init(struct pario_port *port, struct timer_handle *th, struct Library *SysBase)
{
  proto_handle_t *ph;

  ph = AllocMem(sizeof(struct proto_handle), MEMF_CLEAR | MEMF_PUBLIC);
  if(ph == NULL) {
    return NULL;
  }
  ph->port = port;
  ph->timer = th;
  ph->timeout_s  = 0;
  ph->timeout_ms = 500000UL;
  ph->sys_base = SysBase;

  /* control: clk,cflg=out(1) rak=in*/
  *port->ctrl_ddr |= port->clk_mask;
  *port->ctrl_ddr &= ~(port->rak_mask | port->cflg_mask);
  *port->ctrl_port |= port->all_mask;

  /* data: port=0, ddr=0xff (OUT) */
  *port->data_port = 0;
  *port->data_ddr  = 0xff;

  return ph;
}

#undef SysBase
#define SysBase ph->sys_base

void proto_exit(proto_handle_t *ph)
{
  if(ph == NULL) {
    return;
  }
  /* free handle */
  FreeMem(ph, sizeof(struct proto_handle));
}

int proto_reset(proto_handle_t *ph)
{
  // perform reset action
  return proto_action_no_busy(ph, PROTO_ACTION_RESET);
}

int proto_bootloader(proto_handle_t *ph)
{
  // perform bootloader action
  return proto_action_no_busy(ph, PROTO_ACTION_BOOTLOADER);
}

int proto_ping(proto_handle_t *ph)
{
  // perform ping action
  return proto_action(ph, PROTO_ACTION_PING);
}

int proto_knok(proto_handle_t *ph)
{
  // perform knok action
  return proto_action_no_busy(ph, PROTO_ACTION_KNOK);
}

int proto_action(proto_handle_t *ph, UBYTE num)
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);
  if(num > PROTO_MAX_ACTION) {
    return PROTO_RET_INVALID_ACTION;
  }
  UBYTE cmd = PROTO_CMD_ACTION + num;

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_action(port, timeout_flag, cmd);
  timer_stop(ph->timer);

  return result;
}

int proto_action_no_busy(proto_handle_t *ph, UBYTE num)
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);
  if(num > PROTO_MAX_ACTION) {
    return PROTO_RET_INVALID_ACTION;
  }
  UBYTE cmd = PROTO_CMD_ACTION + num;

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_action_no_busy(port, timeout_flag, cmd);
  timer_stop(ph->timer);

  return result;
}

static ASM void bench_cb(REG(d0, int id), REG(a2, struct cb_data *cb))
{
  struct timer_handle *th = (struct timer_handle *)cb->user_data;
  time_stamp_t *ts = &cb->timestamps[id];
  timer_eclock_get(th, ts);
}

int proto_action_bench(proto_handle_t *ph, UBYTE num, ULONG deltas[2])
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);
  if(num > PROTO_MAX_ACTION) {
    return PROTO_RET_INVALID_ACTION;
  }
  UBYTE cmd = PROTO_CMD_ACTION + num;

  struct cb_data cbd = {
    bench_cb,
    ph->timer
  };

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_action_bench(port, timeout_flag, &cbd, cmd);
  timer_stop(ph->timer);

  /* calc deltas */
  time_stamp_t *t0 = &cbd.timestamps[0];
  time_stamp_t *t1 = &cbd.timestamps[1];
  time_stamp_t *t2 = &cbd.timestamps[2];
  time_stamp_t d0, d1;
  timer_eclock_delta(t1, t0, &d0);
  timer_eclock_delta(t2, t1, &d1);
  deltas[0] = timer_eclock_to_us(ph->timer, &d0);
  deltas[1] = timer_eclock_to_us(ph->timer, &d1);

  return result;
}

int proto_function_read_word(proto_handle_t *ph, UBYTE num, UWORD *data)
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);
  if(num > PROTO_MAX_FUNCTION) {
    return PROTO_RET_INVALID_FUNCTION;
  }
  UBYTE cmd = PROTO_CMD_WFUNC_READ + num;

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_read_word(port, timeout_flag, cmd, data);
  timer_stop(ph->timer);

  return result;
}

int proto_function_write_word(proto_handle_t *ph, UBYTE num, UWORD data)
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);
  if(num > PROTO_MAX_FUNCTION) {
    return PROTO_RET_INVALID_FUNCTION;
  }
  UBYTE cmd = PROTO_CMD_WFUNC_WRITE + num;

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_write_word(port, timeout_flag, cmd, &data);
  timer_stop(ph->timer);

  return result;
}

int proto_function_read_long(proto_handle_t *ph, UBYTE num, ULONG *data)
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);
  if(num > PROTO_MAX_FUNCTION) {
    return PROTO_RET_INVALID_FUNCTION;
  }
  UBYTE cmd = PROTO_CMD_LFUNC_READ + num;

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_read_long(port, timeout_flag, cmd, data);
  timer_stop(ph->timer);

  return result;
}

int proto_function_write_long(proto_handle_t *ph, UBYTE num, ULONG data)
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);
  if(num > PROTO_MAX_FUNCTION) {
    return PROTO_RET_INVALID_FUNCTION;
  }
  UBYTE cmd = PROTO_CMD_LFUNC_WRITE + num;

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_write_long(port, timeout_flag, cmd, &data);
  timer_stop(ph->timer);

  return result;
}

int proto_msg_write(proto_handle_t *ph, UBYTE chn, proto_iov_t *msgiov)
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);
  if(chn >= PROTO_MAX_CHANNEL) {
    return PROTO_RET_INVALID_CHANNEL;
  }
  UBYTE dcmd = chn + PROTO_CMD_MSG_WRITE_DATA;
  UBYTE scmd = chn + PROTO_CMD_MSG_WRITE_SIZE;
  UWORD size = (UWORD)msgiov->total_words;
  if(size == 0) {
    return PROTO_RET_OK;
  }

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_write_word(port, timeout_flag, scmd, &size);
  if(result == 0) {
    result = proto_low_write_block(port, timeout_flag, dcmd, &msgiov->first);
  }
  timer_stop(ph->timer);

  return result;
}

int proto_msg_read(proto_handle_t *ph, UBYTE chn, proto_iov_t *msgiov)
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);
  if(chn >= PROTO_MAX_CHANNEL) {
    return PROTO_RET_INVALID_CHANNEL;
  }
  UBYTE dcmd = chn + PROTO_CMD_MSG_READ_DATA;
  UBYTE scmd = chn + PROTO_CMD_MSG_READ_SIZE;
  UWORD size = 0;
  UWORD max_size = (UWORD)msgiov->total_words;
  if(max_size == 0) {
    return PROTO_RET_OK;
  }

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_read_word(port, timeout_flag, scmd, &size);
  if(result == 0) {
    if(size > max_size) {
      result = PROTO_RET_MSG_TOO_LARGE;
    } else if(size == 0) {
      result = PROTO_RET_OK;
    } else {
      result = proto_low_read_block(port, timeout_flag, dcmd, &msgiov->first);
    }
  }
  timer_stop(ph->timer);

  msgiov->total_words = size;
  return result;
}

int proto_msg_write_single(proto_handle_t *ph, UBYTE chn, UBYTE *buf, UWORD num_words)
{
  proto_iov_t msgiov = {
    num_words, /* total size */
    { num_words, /* chunk size */
      buf,       /* chunk pointer */
      0 }          /* next node */
  };
  return proto_msg_write(ph, chn, &msgiov);
}

int proto_msg_read_single(proto_handle_t *ph, UBYTE chn, UBYTE *buf, UWORD *max_words)
{
  proto_iov_t msgiov = {
    *max_words, /* total size */
    { *max_words, /* chunk size */
      buf,        /* chunk pointer */
      0 }         /* next node */
  };
  int result = proto_msg_read(ph, chn, &msgiov);
  /* store returned result size */
  *max_words = msgiov.total_words;
  return result;
}

const char *proto_perror(int res)
{
  switch(res) {
    case PROTO_RET_OK:
      return "OK";
    case PROTO_RET_RAK_INVALID:
      return "RAK invalid";
    case PROTO_RET_TIMEOUT:
      return "timeout";
    case PROTO_RET_SLAVE_ERROR:
      return "slave error";
    case PROTO_RET_INVALID_FUNCTION:
      return "invalid function";
    case PROTO_RET_INVALID_CHANNEL:
      return "invalid channel";
    case PROTO_RET_INVALID_ACTION:
      return "invalid action";
    case PROTO_RET_MSG_TOO_LARGE:
      return "message too large";
    case PROTO_RET_DEVICE_BUSY:
      return "device is busy";
    default:
      return "?";
  }
}
