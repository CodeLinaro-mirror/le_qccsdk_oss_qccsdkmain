#ifndef NTLIB_H_
#define NTLIB_H_

#define rWrite(reg, value) ((*((volatile uint32_t *)(reg))) = (value))
#define rRead(reg) (*(volatile uint32_t *)reg)

#endif /* NTLIB_H_ */
