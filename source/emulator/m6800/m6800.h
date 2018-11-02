/*** m6800: Portable 6800 class emulator *************************************/
#pragma once

#include "arm_emulator.h"

enum {
	M6800_PC=1, M6800_S, M6800_A, M6800_B, M6800_X, M6800_CC,
	M6800_WAI_STATE, M6800_NMI_STATE, M6800_IRQ_STATE };

#define M6800_WAI		8			/* set when WAI is waiting for an interrupt */

#define M6800_IRQ_LINE	0			/* IRQ line number */

/* PUBLIC GLOBALS */
extern int m6800_ICount;

/* PUBLIC FUNCTIONS */
void m6800_init(void);
void m6800_reset(void);
void m6800_exit(void);
int	m6800_execute(int cycles);
unsigned m6800_get_context(void *dst);
void m6800_set_context(void *src);
unsigned m6800_get_reg(int regnum);
void m6800_set_reg(int regnum, unsigned val);
void m6800_set_irq_line(int irqline, int state);
void m6800_set_irq_callback(int (*callback)(int irqline));
const char *m6800_info(void *context, int regnum);
unsigned m6800_dasm(char *buffer, unsigned pc);

/****************************************************************************
 * For now make the 6802 using the m6800 variables and functions
 ****************************************************************************/
#if (HAS_M6802)
#define M6802_A 					M6800_A
#define M6802_B 					M6800_B
#define M6802_PC					M6800_PC
#define M6802_S 					M6800_S
#define M6802_X 					M6800_X
#define M6802_CC					M6800_CC
#define M6802_WAI_STATE 			M6800_WAI_STATE
#define M6802_NMI_STATE 			M6800_NMI_STATE
#define M6802_IRQ_STATE 			M6800_IRQ_STATE

#define M6802_WAI					M6800_WAI
#define M6802_IRQ_LINE				M6800_IRQ_LINE

#define m6802_ICount				m6800_ICount
void m6802_init(void);
void m6802_reset(void *param);
void m6802_exit(void);
int	m6802_execute(int cycles);
unsigned m6802_get_context(void *dst);
void m6802_set_context(void *src);
unsigned m6802_get_reg(int regnum);
void m6802_set_reg(int regnum, unsigned val);
void m6802_set_irq_line(int irqline, int state);
void m6802_set_irq_callback(int (*callback)(int irqline));
const char *m6802_info(void *context, int regnum);
unsigned m6802_dasm(char *buffer, unsigned pc);
#endif


/****************************************************************************
 * For now make the 6808 using the m6800 variables and functions
 ****************************************************************************/
#if (HAS_M6808)
#define M6808_A 					M6800_A
#define M6808_B 					M6800_B
#define M6808_PC					M6800_PC
#define M6808_S 					M6800_S
#define M6808_X 					M6800_X
#define M6808_CC					M6800_CC
#define M6808_WAI_STATE 			M6800_WAI_STATE
#define M6808_NMI_STATE 			M6800_NMI_STATE
#define M6808_IRQ_STATE 			M6800_IRQ_STATE

#define M6808_WAI                   M6800_WAI
#define M6808_IRQ_LINE              M6800_IRQ_LINE

#define m6808_ICount                m6800_ICount
void m6808_init(void);
void m6808_reset(void *param);
void m6808_exit(void);
int	m6808_execute(int cycles);
unsigned m6808_get_context(void *dst);
void m6808_set_context(void *src);
unsigned m6808_get_reg(int regnum);
void m6808_set_reg(int regnum, unsigned val);
void m6808_set_irq_line(int irqline, int state);
void m6808_set_irq_callback(int (*callback)(int irqline));
const char *m6808_info(void *context, int regnum);
unsigned m6808_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************/
/* Read a byte from given memory location									*/
/****************************************************************************/
#define M6800_RDMEM(Addr) ((unsigned)cpu_readmem16(Addr))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define M6800_WRMEM(Addr,Value) (cpu_writemem16(Addr,Value))

/****************************************************************************/
/* M6800_RDOP() is identical to M6800_RDMEM() except it is used for reading */
/* opcodes. Since all of our ROM is internal, this makes things a lot       */
/* faster.                                                                  */
/****************************************************************************/
#define M6800_RDOP(Addr) ((unsigned)cpu_readop(Addr))

/****************************************************************************/
/* M6800_RDOP_ARG() is identical to M6800_RDOP() but it's used for reading  */
/* opcode arguments.*/
#define M6800_RDOP_ARG(Addr) ((unsigned)cpu_readop_arg(Addr))


unsigned Dasm680x(int subtype, char *buf, unsigned pc);
