#ifndef __UK_DRIVER_SDDF_SERIAL_H__
#define __UK_DRIVER_SDDF_SERIAL_H__
int uk_sddf_serial_init(void);
__ssz uk_sddf_serial_write(const char *buf, __sz len);
__ssz uk_sddf_serial_emerg_write(const char *buf, __sz len);
__ssz uk_sddf_serial_read(char *buf, __sz len);
__ssz uk_sddf_serial_read_blocking(char *buf, __sz len);
#endif
