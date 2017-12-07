#include "autoconf.h"
#include "types.h"
#include "arch.h"

#define DEBUG 1

#include "debug.h"

#include "system.h"
#include "rominfo.h"
#include "fwid.h"
#include "uart.h"
#include "uartutil.h"
#include "base_reg.h"
#include "proto.h"
#include "proto_shared.h"
#include "status.h"
#include "buffer.h"

#include "handler.h"
#include "handler_reg.h"
#include "hnd_echo.h"
#include "hnd_null.h"

#include "driver.h"
#include "driver_list.h"


// define my app id
BASE_REG_APPID(FWID_TEST_HANDLER)

// set register table
REG_TABLE_SETUP(handler)

// handler
HANDLER_TABLE_BEGIN
  HANDLER_TABLE_ENTRY(echo),
  HANDLER_TABLE_ENTRY(echo),
  HANDLER_TABLE_ENTRY(null)
HANDLER_TABLE_END

// driver
DRIVER_TABLE_BEGIN
  DRIVER_TABLE_ENTRY(null),
#ifdef CONFIG_DRIVER_ENC28J60
  DRIVER_TABLE_ENTRY(enc28j60),
#endif
#ifdef CONFIG_DRIVER_SDCARD
  DRIVER_TABLE_ENTRY(sdcard),
#endif
DRIVER_TABLE_END


int main(void)
{
  system_init();

  uart_init();
  uart_send_pstring(PSTR("parbox: test-handler!"));
  uart_send_crlf();

  rom_info();

  DC('+');
  proto_init(PROTO_STATUS_INIT);
  status_init();
  buffer_init();
  DRIVER_INIT();
  HANDLER_INIT();
  DC('-'); DNL;

  while(1) {
    system_wdt_reset();
    proto_handle();
    status_handle();
    DRIVER_WORK();
    HANDLER_WORK();
  }

  return 0;
}