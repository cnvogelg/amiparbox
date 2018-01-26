#define __NOLIBBASE__
#include <proto/exec.h>

#include "autoconf.h"
#include "compiler.h"
#include "debug.h"

#include "paloma.h"

int paloma_init(paloma_handle_t *ph, pamela_handle_t *pm)
{
  ph->pamela = pm;
  ph->pamela_error = PAMELA_OK;

  return PALOMA_OK;
}

void paloma_exit(paloma_handle_t *ph)
{
  /* on palome exit return device into knok mode */
  proto_handle_t *proto = pamela_get_proto(ph->pamela);
  proto_reset(proto, 0);
}

const char *paloma_perror(int res)
{
  switch(res) {
    case PALOMA_OK:
      return "paloma: OK";
    case PALOMA_ERROR_IN_PAMELA:
      return "paloma: error in pamela";
    default:
      return "paloma: unknown error!";
  }
}