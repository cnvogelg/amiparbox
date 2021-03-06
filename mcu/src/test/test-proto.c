#include "autoconf.h"
#include "types.h"
#include "arch.h"

#include "debug.h"

#include "uart.h"
#include "uartutil.h"
#include "rominfo.h"
#include "system.h"
#include "led.h"
#include "timer.h"

#include "knok.h"
#include "proto.h"
#include "proto_shared.h"
#include "proto_test_shared.h"
#include "proto_api.h"

#include "fwid.h"
#include "fw_info.h"
#include "spi.h"

FW_INFO(FWID_TEST_PROTO, VERSION_TAG)

#define MAX_WORDS  512
u08 buf[MAX_WORDS * 2];

u16 test_flags = 0;
u16 the_size = MAX_WORDS;
u32 the_offset = 0;
u16 word_val = 0x4711;
u32 long_val = 0xdeadbeef;
u08 enter_busy = 0;

static void busy_loop(void)
{
  uart_send_pstring(PSTR("busy:begin"));
  proto_set_busy();

  // wait for a second
  for(int i=0;i<5;i++) {
    system_wdt_reset();
    uart_send('.');
    timer_delay(200);
  }

  proto_clr_busy();
  uart_send_pstring(PSTR("end"));
  uart_send_crlf();
}

// action handler

void proto_api_action(u08 num)
{
#ifdef FLAVOR_DEBUG
  uart_send_pstring(PSTR("action:"));
  uart_send_hex_byte(num);
  uart_send_crlf();
#endif

  // triger signal
  switch(num) {
    case PROTO_ACTION_TEST_SIGNAL:
      uart_send_pstring(PSTR("signal!"));
      uart_send_crlf();
      proto_trigger_signal();
      break;
    case PROTO_ACTION_TEST_BUSY_LOOP:
      enter_busy = 1;
      break;
  }
}

// function handler

u16  proto_api_wfunc_read(u08 num)
{
  u16 val = 0xbeef;
  switch(num) {
    case PROTO_WFUNC_READ_TEST_FW_ID:
      val = FW_GET_ID();
      break;
    case PROTO_WFUNC_READ_TEST_FW_VERSION:
      val = FW_GET_VERSION();
      break;
    case PROTO_WFUNC_READ_TEST_MACHTAG:
      val = FW_GET_MACHTAG();
      break;
    case PROTO_WFUNC_READ_TEST_FLAGS:
      val = test_flags;
      break;
    case PROTO_WFUNC_READ_TEST_VALUE:
      val = word_val;
      break;
    case PROTO_WFUNC_READ_TEST_MAX_WORDS:
      val = MAX_WORDS;
      break;
  }
#ifdef FLAVOR_DEBUG
  uart_send_pstring(PSTR("wfunc_read:"));
  uart_send_hex_byte(num);
  uart_send('=');
  uart_send_hex_word(val);
  uart_send_crlf();
#endif
  return val;
}

void proto_api_wfunc_write(u08 num, u16 val)
{
  switch(num) {
    case PROTO_WFUNC_WRITE_TEST_FLAGS:
      test_flags = val;
      break;
    case PROTO_WFUNC_WRITE_TEST_VALUE:
      word_val = val;
      break;
  }

#ifdef FLAVOR_DEBUG
  uart_send_pstring(PSTR("wfunc_write:"));
  uart_send_hex_byte(num);
  uart_send('=');
  uart_send_hex_word(val);
  uart_send_crlf();
#endif
}

u32 proto_api_lfunc_read(u08 num)
{
  u32 val = 0xcafebabe;
  switch(num) {
    case PROTO_LFUNC_READ_TEST_VALUE:
      val = long_val;
      break;
    case PROTO_LFUNC_READ_TEST_OFFSET:
      val = the_offset;
      break;
  }
#ifdef FLAVOR_DEBUG
  uart_send_pstring(PSTR("lfunc_read:"));
  uart_send_hex_byte(num);
  uart_send('=');
  uart_send_hex_long(val);
  uart_send_crlf();
#endif
  return val;
}

void proto_api_lfunc_write(u08 num, u32 val)
{
  switch(num) {
    case PROTO_LFUNC_WRITE_TEST_VALUE:
      long_val = val;
      break;
  }
#ifdef FLAVOR_DEBUG
  uart_send_pstring(PSTR("lfunc_write:"));
  uart_send_hex_byte(num);
  uart_send('=');
  uart_send_hex_long(val);
  uart_send_crlf();
#endif
}

// message i/o

u08 *proto_api_read_msg_begin(u08 chan, u16 *size)
{
  *size = the_size;

#ifdef FLAVOR_DEBUG
  uart_send_pstring(PSTR("msg_read:{"));
  uart_send_hex_byte(chan);
  uart_send('+');
  uart_send_hex_word(*size);
#endif

  if(test_flags & PROTO_TEST_FLAGS_USE_SPI) {
    spi_enable_cs0();
    return NULL;
  } else {
    return buf;
  }
}

void proto_api_read_msg_done(u08 chan)
{
  if(test_flags & PROTO_TEST_FLAGS_USE_SPI) {
    spi_disable_cs0();
  }

#ifdef FLAVOR_DEBUG
  uart_send('}');
  uart_send_crlf();
#endif
}

u08 *proto_api_write_msg_begin(u08 chan,u16 *size)
{
  *size = the_size;

#ifdef FLAVOR_DEBUG
  uart_send_pstring(PSTR("msg_write:{"));
  uart_send_hex_byte(chan);
  uart_send('+');
  uart_send_hex_word(*size);
#endif

  if(test_flags & PROTO_TEST_FLAGS_USE_SPI) {
    spi_enable_cs0();
    return NULL;
  } else {
    return buf;
  }
}

void proto_api_write_msg_done(u08 chan)
{
  if(test_flags & PROTO_TEST_FLAGS_USE_SPI) {
    spi_disable_cs0();
  }

#ifdef FLAVOR_DEBUG
  uart_send('}');
  uart_send_crlf();
#endif
}

// extended commands

void proto_api_chn_set_offset(u08 chan, u32 offset)
{
#ifdef FLAVOR_DEBUG
  uart_send_pstring(PSTR("offset="));
  uart_send_hex_long(offset);
  uart_send_crlf();
#endif
  the_offset = offset;
}

u16  proto_api_chn_get_rx_size(u08 chan)
{
#ifdef FLAVOR_DEBUG
  uart_send_pstring(PSTR("rx_size:"));
  uart_send_hex_word(the_size);
  uart_send_crlf();
#endif
  return the_size;
}

void proto_api_chn_set_tx_size(u08 chan, u16 size)
{
#ifdef FLAVOR_DEBUG
  uart_send_pstring(PSTR("tx_size="));
  uart_send_hex_word(size);
  uart_send_crlf();
#endif
  if(size <= MAX_WORDS) {
    the_size = size;
  }
}

void proto_api_chn_cancel_transfer(u08 chan)
{
#ifdef FLAVOR_DEBUG
  uart_send_pstring(PSTR("cancel_transfer"));
  uart_send_crlf();
#endif
  test_flags |= PROTO_TEST_FLAGS_CANCEL_TRANSFER;
}

// ----- main -----

int main(void)
{
  system_init();
  //led_init();
  spi_init();

  uart_init();
  uart_send_pstring(PSTR("parbox: test-proto!"));
  uart_send_crlf();

  rom_info();

  knok_main();

  proto_init();
  proto_first_cmd();
  while(1) {
      proto_handle();
      system_wdt_reset();

      if(enter_busy) {
        busy_loop();
        enter_busy = 0;
      }
  }

  return 0;
}
