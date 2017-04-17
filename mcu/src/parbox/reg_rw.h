#ifndef REG_RW_H
#define REG_RW_H

extern u16 reg_rw_get(u08 reg_num);
extern void reg_rw_set(u08 reg_num, u16 val);

#define REG_RW_FLAG_NONE      0
#define REG_RW_FLAG_BYTE      1
#define REG_RW_FLAG_FUNC      2

typedef u16 (*reg_rw_get_func_t)(void);
typedef void (*reg_rw_set_func_t)(u16 v);

struct reg_rw_table_entry {
  union {
    u16 *wptr;
    u08 *cptr;
  } ptr;
  reg_rw_get_func_t get_func;
  reg_rw_set_func_t set_func;
  u08 flags;
};
typedef struct reg_rw_table_entry reg_rw_table_entry_t;

extern const u08 reg_rw_table_size ROM_ATTR;
extern const reg_rw_table_entry_t reg_rw_table[] ROM_ATTR;

#define REG_RW_TABLE_SIZE           sizeof(reg_rw_table)/sizeof(reg_rw_table[0])

#define REG_RW_TABLE_BEGIN          const reg_rw_table_entry_t reg_rw_table[] ROM_ATTR = {
#define REG_RW_TABLE_END            }; const u08 reg_rw_table_size = REG_RW_TABLE_SIZE;

#define REG_RW_TABLE_RAM_W(v)         { {.wptr = &v}, .flags=REG_RW_FLAG_NONE }
#define REG_RW_TABLE_RAM_B(v)         { {.cptr = &v}, .flags=REG_RW_FLAG_BYTE }
#define REG_RW_TABLE_FUNC(g,s)        { .get_func=g, .set_func=s, .flags=REG_RW_FLAG_FUNC }

#endif
