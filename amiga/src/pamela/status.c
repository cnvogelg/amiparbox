#define __NOLIBBASE__
#include <proto/exec.h>

#include "autoconf.h"
#include "compiler.h"
#include "debug.h"

#include "proto_shared.h"
#include "proto.h"
#include "reg.h"
#include "status.h"

void status_init(status_data_t *data)
{
  data->pending_channel = STATUS_NO_CHANNEL;
  data->event_mask = STATUS_NO_EVENTS;
  data->flags = STATUS_FLAGS_INIT;
}

int status_update(proto_handle_t *ph, status_data_t *data)
{
  int res = PROTO_RET_OK;

  // get state byte
  UBYTE state = proto_get_status(ph);

  // decode state byte
  // a read is pending
  if((state & PROTO_STATUS_READ_PENDING) == PROTO_STATUS_READ_PENDING) {
    data->pending_channel = state & PROTO_STATUS_CHANNEL_MASK;
    data->event_mask = STATUS_NO_EVENTS;
    data->flags &= ~STATUS_FLAGS_EVENTS;
    data->flags |= STATUS_FLAGS_PENDING;
  } else {
    data->flags = 0;
    data->pending_channel = STATUS_NO_CHANNEL;
    data->event_mask = STATUS_NO_EVENTS;
    if(state & PROTO_STATUS_BOOTLOADER) {
      data->flags |= STATUS_FLAGS_BOOTLOADER;
    }
    if(state & PROTO_STATUS_ATTACHED) {
      data->flags |= STATUS_FLAGS_ATTACHED;
    }
    if(state & PROTO_STATUS_EVENTS) {
      data->flags |= STATUS_FLAGS_EVENTS;

      // try to read error mask
      UWORD e;
      res = reg_base_get_event_mask(ph, &e);
      data->event_mask = (UBYTE)e;
    }
  }

  return res;
}
