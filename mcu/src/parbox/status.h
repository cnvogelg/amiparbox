#ifndef STATUS_H
#define STATUS_H

#define STATUS_NO_ERROR 0
#define STATUS_NO_CHANNEL 0xff

extern void status_init(void);
extern void status_update(void);
extern void status_handle(void);
extern void status_restore(void);

extern void status_set_error(u08 error);
extern u08  status_clear_error(void);

extern void status_attach(void);
extern void status_detach(void);

extern void status_set_pending(u08 channel);
extern void status_clear_pending(void);
extern u08  status_is_pending(void);

#endif