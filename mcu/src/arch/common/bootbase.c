#include "autoconf.h"
#include "types.h"
#include "arch.h"

#include "bootbase.h"
#include "bootloader_shared.h"
#include "proto_shared.h"
#include "pablo.h"
#include "uart.h"
#include "proto_low.h"
#include "proto.h"
#include "proto_api.h"
#include "flash.h"
#include "machtag.h"
#include "fwid.h"

#include "uartutil.h"

static u08 status;
static u16 page_words;
static u32 page_addr;
static u08 *page_buf;


// ----- actions -----

void proto_api_acion(u08 num)
{
  /* no extra action */
}

// ----- functions -----

u16  proto_api_wfunc_read(u08 num)
{
  switch(num) {
    case PROTO_WFUNC_READ_BOOT_FW_ID:
      return FWID_BOOTLOADER_PABLO;
    case PROTO_WFUNC_READ_BOOT_MACHTAG:
      return MACHTAG;
    case PROTO_WFUNC_READ_BOOT_FW_VERSION:
      return VERSION_TAG;
    case PROTO_WFUNC_READ_BOOT_ROM_CRC:
      return pablo_get_rom_crc();
    case PROTO_WFUNC_READ_BOOT_ROM_MACHTAG:
      return pablo_get_mach_tag();
    case PROTO_WFUNC_READ_BOOT_ROM_FW_VERSION:
      return pablo_get_rom_version();
    case PROTO_WFUNC_READ_BOOT_ROM_FW_ID:
      return pablo_get_rom_fw_id();
    case PROTO_WFUNC_READ_BOOT_PAGE_WORDS:
      return page_words;
    default:
      return 0;
  }
}

u32 proto_api_lfunc_read(u08 num)
{
  /* only ROM size for now */
  uart_send('r');
  return CONFIG_MAX_ROM;
}

void proto_api_lfunc_write(u08 num, u32 val)
{
  /* only page addr for now */
  uart_send('A');
  page_addr = val;
}

void proto_api_action(u08 num)
{
}

// ----- main -----

u08 bootbase_init(u16 page_size, u08 *buf_ptr)
{
  page_words = page_size / 2;
  page_buf = buf_ptr;

  // say hello
  uart_init();
  uart_send('P');

  // setup proto
  proto_init();
  uart_send('A');

  // check if bootloader command is set - if not enter app
  u08 cmd = proto_low_get_cmd();
  if(cmd != (PROTO_CMD_ACTION | PROTO_ACTION_BOOTLOADER)) {
    // check crc
    uart_send('B');
    u16 crc = pablo_check_rom_crc();
    if(crc == 0) {
      uart_send('L');
      // ensure that mach_tag matches in pablo footer
      u16 rom_mach_tag = pablo_get_mach_tag();
      if(rom_mach_tag == MACHTAG) {
        uart_send('O');
        // run app if valid -> run it
        return BOOTBASE_RET_RUN_APP;
      }
    }
  }
  else {
    // reply to bootloader command
    uart_send('-');
    proto_low_action();
    proto_low_end();
  }

  // wait if computer is off
  if(cmd == 0) {
    uart_send('o');
    while(proto_low_get_cmd() == 0) {
      boot_wdt_reset();
    }
  }

  // enter main loop
  uart_send(':');
  while(1) {
    proto_handle_mini();
    boot_wdt_reset();
  }
  return BOOTBASE_RET_CMD_LOOP;
}

// msg i/o is used to transfer page data - channel is ignored in bootloader

u08 *proto_api_read_msg_begin(u08 chan, u16 *size)
{
  *size = page_words;
  uart_send('r');
  flash_read_page(page_addr, page_buf);
  return page_buf;
}

void proto_api_read_msg_done(u08 chan)
{
  uart_send('.');
}

u08 *proto_api_write_msg_begin(u08 chan, u16 *size)
{
  *size = page_words;
  uart_send('w');
  return page_buf;
}

void proto_api_write_msg_done(u08 chan)
{
  uart_send('(');
  flash_program_page(page_addr, page_buf);
  uart_send(')');
  status = BOOT_STATUS_OK;
}

void proto_low_write_block_spi(u16 num_words)
{
}

void proto_low_read_block_spi(u16 num_words)
{
}
