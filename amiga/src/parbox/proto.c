#include <proto/exec.h>

#include "autoconf.h"
#include "compiler.h"
#include "debug.h"
#include "pario.h"
#include "timer.h"
#include "proto_low.h"
#include "proto.h"

/* proto signals */
#define clk_mask    pout_mask
#define rak_mask    busy_mask
#define pend_mask   sel_mask

#define DDR_DATA_OUT  0xff
#define DDR_DATA_IN   0x00

struct proto_handle {
    struct pario_port   *port;
    struct timer_handle *timer;
    ULONG                timeout_s;
    ULONG                timeout_ms;
};

struct proto_handle *proto_init(struct pario_port *port, struct timer_handle *th)
{
  struct proto_handle *ph;

  ph = AllocMem(sizeof(struct proto_handle), MEMF_CLEAR);
  if(ph == NULL) {
    return NULL;
  }
  ph->port = port;
  ph->timer = th;
  ph->timeout_s  = 0;
  ph->timeout_ms = 500000UL;

  /* control: clk=out(1) rak,pend=in*/
  *port->ctrl_ddr |= port->clk_mask;
  *port->ctrl_ddr &= ~(port->rak_mask | port->pend_mask);
  *port->ctrl_port |= port->all_mask;

  /* data: port=0, ddr=0xff (OUT) */
  *port->data_port = CMD_IDLE;
  *port->data_ddr  = 0xff;

  return ph;
}

void proto_exit(struct proto_handle *ph)
{
  if(ph == NULL) {
    return;
  }
  /* free handle */
  FreeMem(ph, sizeof(struct proto_handle));
}

int proto_ping(struct proto_handle *ph)
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_ping(port, timeout_flag);
  timer_stop(ph->timer);

  return result;
}

int proto_test_read(struct proto_handle *ph, UBYTE *data)
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_test_read(port, timeout_flag, data);
  timer_stop(ph->timer);

  return result;
}

int proto_test_write(struct proto_handle *ph, UBYTE *data)
{
  struct pario_port *port = ph->port;
  volatile BYTE *timeout_flag = timer_get_flag(ph->timer);

  timer_start(ph->timer, ph->timeout_s, ph->timeout_ms);
  int result = proto_low_test_write(port, timeout_flag, data);
  timer_stop(ph->timer);

  return result;
}
