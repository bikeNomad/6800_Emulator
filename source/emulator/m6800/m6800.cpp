/*

    Chip                RAM     NVRAM   ROM     SCI     r15-f   ports
    -----------------------------------------------------------------
    MC6800              -       -       -       no      no      4
    MC6802              128     32      -       no      no      4
    MC6802NS            128     -       -       no      no      4
    MC6808              -       -       -       no      no      4

    MC6801              128     64      2K      yes     no      4
    MC68701             128     64      -       yes     no      4
    MC6803              128     64      -       yes     no      4

    MC6801U4            192     32      4K      yes     yes     4
    MC6803U4            192     32      -       yes     yes     4

    HD6801              128     64      2K      yes     no      4
    HD6301V             128     -       4K      yes     no      4
    HD63701V            192     -       4K      yes     no      4
    HD6303R             128     -       -       yes     no      4

    HD6301X             192     -       4K      yes     yes     6
    HD6301Y             256     -       16K     yes     yes     6
    HD6303X             192     -       -       yes     yes     6
    HD6303Y             256     -       -       yes     yes     6

    NSC8105
    MS2010-A

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fsl_common.h"
#include "pin_mux.h"

#include "cpuintrf.h"

#include "m6800.h"
#include "arm_emulator.h"
#include "mamedbg.h"


#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

/* 6800 Registers */
typedef struct m6800_Regs
{
	PAIR	ppc;			/* Previous program counter */
	PAIR	pc; 			/* Program counter */
	PAIR	s;				/* Stack pointer */
	PAIR	x;				/* Index register */
	PAIR	d;				/* Accumulators */
	UINT8	cc; 			/* Condition codes */
	UINT8	wai_state;		/* WAI opcode state ,(or sleep opcode state) */
	UINT8	nmi_state;		/* NMI line state */
	UINT8	irq_state;		/* IRQ line state */

	int		(*irq_callback)(int irqline);
	int 	extra_cycles;	/* cycles used for interrupts */
	void	(* const * insn)(void);	/* instruction table */
	const UINT8 *cycles;			/* clock cycle of instruction table */
}  m6800_Regs;

/* 680x registers */
static m6800_Regs m6800;

#define m6801   m6800
#define m6802   m6800
#define m6803	m6800
#define m6808	m6800
#define hd63701 m6800
#define nsc8105 m6800

#define	pPPC	m6800.ppc
#define pPC 	m6800.pc
#define pS		m6800.s
#define pX		m6800.x
#define pD		m6800.d

#define PC		m6800.pc.w.l
#define PCD		m6800.pc.d
#define S		m6800.s.w.l
#define SD		m6800.s.d
#define X		m6800.x.w.l
#define D		m6800.d.w.l
#define A		m6800.d.b.h
#define B		m6800.d.b.l
#define CC		m6800.cc

static PAIR ea; 		/* effective address */
#define EAD ea.d
#define EA	ea.w.l

/* public globals */
int m6800_ICount=50000;


/* DS -- THESE ARE RE-DEFINED IN m6800.h TO RAM, ROM or FUNCTIONS IN cpuintrf.c */
#define RM				M6800_RDMEM
#define WM				M6800_WRMEM
#define M_RDOP			M6800_RDOP
#define M_RDOP_ARG		M6800_RDOP_ARG

/* macros to access memory */
#define IMMBYTE(b)	b = M_RDOP_ARG(PCD); PC++
#define IMMWORD(w)	w.d = (M_RDOP_ARG(PCD)<<8) | M_RDOP_ARG((PCD+1)&0xffff); PC+=2

#define PUSHBYTE(b) WM(SD,b); --S
#define PUSHWORD(w) WM(SD,w.b.l); --S; WM(SD,w.b.h); --S
#define PULLBYTE(b) S++; b = RM(SD)
#define PULLWORD(w) S++; w.d = RM(SD)<<8; S++; w.d |= RM(SD)


/* execute one instruction using table lookup */
#define ONE_MORE_INSN() {					\
	UINT8 ireg; 							\
	pPPC = pPC; 							\
	CALL_MAME_DEBUG;						\
	ireg=M_RDOP(PCD);						\
	PC++;									\
	(*m6800.insn[ireg])();					\
}

/* check the IRQ lines for pending interrupts */
#define CHECK_IRQ_LINES() {										\
	if( !(CC & 0x10) )											\
	{															\
		if( m6800.irq_state != CLEAR_LINE )		\
		{	/* standard IRQ */									\
			ENTER_INTERRUPT("M6800#%d take IRQ1\n",0xfff8);		\
			if( m6800.irq_callback )							\
				(void)(*m6800.irq_callback)(M6800_IRQ_LINE);	\
		}														\
	}															\
}

/* CC masks                       HI NZVC
								7654 3210	*/
#define CLR_HNZVC	CC&=0xd0
#define CLR_NZV 	CC&=0xf1
#define CLR_HNZC	CC&=0xd2
#define CLR_NZVC	CC&=0xf0
#define CLR_Z		CC&=0xfb
#define CLR_ZC		CC&=0xfa
#define CLR_C		CC&=0xfe

/* macros for CC -- CC bits affected should be reset before calling */
#define SET_Z(a)		if(!(a))SEZ
#define SET_Z8(a)		SET_Z((UINT8)(a))
#define SET_Z16(a)		SET_Z((UINT16)(a))
#define SET_N8(a)		CC|=(((a)&0x80)>>4)
#define SET_N16(a)		CC|=(((a)&0x8000)>>12)
#define SET_H(a,b,r)	CC|=((((a)^(b)^(r))&0x10)<<1)
#define SET_C8(a)		CC|=(((a)&0x100)>>8)
#define SET_C16(a)		CC|=(((a)&0x10000)>>16)
#define SET_V8(a,b,r)	CC|=((((a)^(b)^(r)^((r)>>1))&0x80)>>6)
#define SET_V16(a,b,r)	CC|=((((a)^(b)^(r)^((r)>>1))&0x8000)>>14)

static const UINT8 flags8i[256]=	 /* increment */
{
0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x0a,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
};
static const UINT8 flags8d[256]= /* decrement */
{
0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
};
#define SET_FLAGS8I(a)		{CC|=flags8i[(a)&0xff];}
#define SET_FLAGS8D(a)		{CC|=flags8d[(a)&0xff];}

/* combos */
#define SET_NZ8(a)			{SET_N8(a);SET_Z8(a);}
#define SET_NZ16(a)			{SET_N16(a);SET_Z16(a);}
#define SET_FLAGS8(a,b,r)	{SET_N8(r);SET_Z8(r);SET_V8(a,b,r);SET_C8(r);}
#define SET_FLAGS16(a,b,r)	{SET_N16(r);SET_Z16(r);SET_V16(a,b,r);SET_C16(r);}

/* for treating an UINT8 as a signed INT16 */
#define SIGNED(b) ((INT16)((b)&0x80?(b)|0xff00:(b)))

/* Macros for addressing modes */
#define DIRECT IMMBYTE(EAD)
#define IMM8 EA=PC++
#define IMM16 {EA=PC;PC+=2;}
#define EXTENDED IMMWORD(ea)
#define INDEXED {EA=X+(UINT8)M_RDOP_ARG(PCD);PC++;}

/* macros to set status flags */
#define SEC CC|=0x01
#define CLC CC&=0xfe
#define SEZ CC|=0x04
#define CLZ CC&=0xfb
#define SEN CC|=0x08
#define CLN CC&=0xf7
#define SEV CC|=0x02
#define CLV CC&=0xfd
#define SEH CC|=0x20
#define CLH CC&=0xdf
#define SEI CC|=0x10
#define CLI CC&=~0x10


/* macros for convenience */
#define DIRBYTE(b) {DIRECT;b=RM(EAD);}
#define DIRWORD(w) {DIRECT;w.d=RM16(EAD);}
#define EXTBYTE(b) {EXTENDED;b=RM(EAD);}
#define EXTWORD(w) {EXTENDED;w.d=RM16(EAD);}

#define IDXBYTE(b) {INDEXED;b=RM(EAD);}
#define IDXWORD(w) {INDEXED;w.d=RM16(EAD);}

/* Macros for branch instructions */
// TODO(nk): define CHANGE_PC()
#define CHANGE_PC()
#define BRANCH(f) {IMMBYTE(t);if(f){PC+=SIGNED(t);CHANGE_PC();}}
#define NXORV  ((CC&0x08)^((CC&0x02)<<2))
#define NXORC  ((CC&0x08)^((CC&0x01)<<3))

/* Note: don't use 0 cycles here for invalid opcodes so that we don't */
/* hang in an infinite loop if we hit one */
#define XX 5 // invalid opcode unknown cc

static const UINT8 cycles_6800[] =
{
		/* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
	/*0*/ XX, 2,XX,XX,XX,XX, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2,
	/*1*/  2, 2,XX,XX,XX,XX, 2, 2,XX, 2,XX, 2,XX,XX,XX,XX,
	/*2*/  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	/*3*/  4, 4, 4, 4, 4, 4, 4, 4,XX, 5,XX,10,XX,XX, 9,12,
	/*4*/  2,XX,XX, 2, 2,XX, 2, 2, 2, 2, 2,XX, 2, 2,XX, 2,
	/*5*/  2,XX,XX, 2, 2,XX, 2, 2, 2, 2, 2,XX, 2, 2,XX, 2,
	/*6*/  7,XX,XX, 7, 7,XX, 7, 7, 7, 7, 7,XX, 7, 7, 4, 7,
	/*7*/  6,XX,XX, 6, 6,XX, 6, 6, 6, 6, 6,XX, 6, 6, 3, 6,
	/*8*/  2, 2, 2,XX, 2, 2, 2, 3, 2, 2, 2, 2, 3, 8, 3, 4,
	/*9*/  3, 3, 3,XX, 3, 3, 3, 4, 3, 3, 3, 3, 4, 6, 4, 5,
	/*A*/  5, 5, 5,XX, 5, 5, 5, 6, 5, 5, 5, 5, 6, 8, 6, 7,
	/*B*/  4, 4, 4,XX, 4, 4, 4, 5, 4, 4, 4, 4, 5, 9, 5, 6,
	/*C*/  2, 2, 2,XX, 2, 2, 2, 3, 2, 2, 2, 2,XX,XX, 3, 4,
	/*D*/  3, 3, 3,XX, 3, 3, 3, 4, 3, 3, 3, 3,XX,XX, 4, 5,
	/*E*/  5, 5, 5,XX, 5, 5, 5, 6, 5, 5, 5, 5,XX,XX, 6, 7,
	/*F*/  4, 4, 4,XX, 4, 4, 4, 5, 4, 4, 4, 4,XX,XX, 5, 6
};


INLINE UINT32 RM16( UINT32 Addr )
{
	UINT32 result = RM(Addr) << 8;
	return result | RM((Addr+1)&0xffff);
}

INLINE void WM16( UINT32 Addr, PAIR *p )
{
	WM( Addr, p->b.h );
	WM( (Addr+1)&0xffff, p->b.l );
}

/* IRQ enter */
static void ENTER_INTERRUPT(const char *message,UINT16 irq_vector)
{
	if( m6800.wai_state & (M6800_WAI) )
	{
		if( m6800.wai_state & M6800_WAI )
			m6800.extra_cycles += 4;
		m6800.wai_state &= ~(M6800_WAI);
	}
	else
	{
		PUSHWORD(pPC);
		PUSHWORD(pX);
		PUSHBYTE(A);
		PUSHBYTE(B);
		PUSHBYTE(CC);
		m6800.extra_cycles += 12;
	}
	SEI;
	PCD = RM16( irq_vector );
	CHANGE_PC();
}

/* include the opcode prototypes and function pointer tables */
#include <6800tbl.h>

/* include the opcode functions */
#include <6800ops.h>

/****************************************************************************
 * Reset registers to their initial values
 ****************************************************************************/

void m6800_init(void)
{
	m6800.insn = m6800_insn;
	m6800.cycles = cycles_6800;
}

void m6800_reset(void *param)
{
	SEI;				/* IRQ disabled */
	PCD = RM16( 0xfffe );
	CHANGE_PC();

	m6800.wai_state = 0;
	m6800.nmi_state = 0;
	m6800.irq_state = 0;
}

/****************************************************************************
 * Shut down CPU emulation
 ****************************************************************************/
void m6800_exit(void)
{
	/* nothing to do */
}

/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/
unsigned m6800_get_context(void *dst)
{
	if( dst )
		*(m6800_Regs*)dst = m6800;
	return sizeof(m6800_Regs);
}


/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void m6800_set_context(void *src)
{
	if( src )
		m6800 = *(m6800_Regs*)src;
	CHANGE_PC();
	CHECK_IRQ_LINES(); /* HJB 990417 */
}


/****************************************************************************
 * Return a specific register
 ****************************************************************************/
unsigned m6800_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC: return PC;
		case M6800_PC: return m6800.pc.w.l;
		case REG_SP: return S;
		case M6800_S: return m6800.s.w.l;
		case M6800_CC: return m6800.cc;
		case M6800_A: return m6800.d.b.h;
		case M6800_B: return m6800.d.b.l;
		case M6800_X: return m6800.x.w.l;
		case M6800_NMI_STATE: return m6800.nmi_state;
		case M6800_IRQ_STATE: return m6800.irq_state;
		case REG_PREVIOUSPC: return m6800.ppc.w.l;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					return ( RM( offset ) << 8 ) | RM( offset+1 );
			}
	}
	return 0;
}


/****************************************************************************
 * Set a specific register
 ****************************************************************************/
void m6800_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC: PC = val; CHANGE_PC(); break;
		case M6800_PC: m6800.pc.w.l = val; break;
		case REG_SP: S = val; break;
		case M6800_S: m6800.s.w.l = val; break;
		case M6800_CC: m6800.cc = val; break;
		case M6800_A: m6800.d.b.h = val; break;
		case M6800_B: m6800.d.b.l = val; break;
		case M6800_X: m6800.x.w.l = val; break;
		case M6800_NMI_STATE: m6800_set_irq_line(IRQ_LINE_NMI, val); break;
		case M6800_IRQ_STATE: m6800_set_irq_line(M6800_IRQ_LINE,val); break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
				{
					WM( offset, (val >> 8) & 0xff );
					WM( offset+1, val & 0xff );
				}
			}
	}
}


void m6800_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
	{
		if (m6800.nmi_state == state) return;
		LOG(("M6800#%d set_nmi_line %d \n", cpu_getactivecpu(), state));
		m6800.nmi_state = state;
		if (state == CLEAR_LINE) return;

		/* NMI */
		ENTER_INTERRUPT("M6800#%d take NMI\n",0xfffc);
	}
	else
	{
		if (m6800.irq_state == state) return;
		LOG(("M6800#%d set_irq_line %d,%d\n", cpu_getactivecpu(), irqline, state));
		m6800.irq_state = state;
		if (state == CLEAR_LINE) return;
		CHECK_IRQ_LINES(); /* HJB 990417 */
	}
}

void m6800_set_irq_callback(int (*callback)(int irqline))
{
	m6800.irq_callback = callback;
}

/****************************************************************************
 * Execute cycles CPU cycles. Return number of cycles really executed
 ****************************************************************************/
int m6800_execute(int cycles)
{
	UINT8 ireg;
	m6800_ICount = cycles;

	m6800.extra_cycles = 0;

	// interrupt lines are active-low
	uint16_t lastInterruptLines = BOARD_NMI_MASK | BOARD_IRQ_MASK;

	do
	{
		// TODO(nk): set up IRQ lines as edge-triggered interrupts
		uint16_t interruptLines = readInterruptLines();
		if (interruptLines ^ lastInterruptLines) {
			m6800_set_irq_line(IRQ_LINE_NMI, (interruptLines & BOARD_NMI_MASK) ? CLEAR_LINE : ASSERT_LINE);
			m6800_set_irq_line(M6800_IRQ_LINE, (interruptLines & BOARD_IRQ_MASK) ? CLEAR_LINE : ASSERT_LINE);
			lastInterruptLines = interruptLines;
		}

		if( m6800.wai_state & M6800_WAI )
		{
			CHECK_IRQ_LINES();
		}
		else
		{
			pPPC = pPC;
			CALL_MAME_DEBUG;
			ireg=M_RDOP(PCD);
			PC++;

			switch( ireg )
			{
				case 0x00: illegal(); break;
				case 0x01: nop(); break;
				case 0x02: illegal(); break;
				case 0x03: illegal(); break;
				case 0x04: illegal(); break;
				case 0x05: illegal(); break;
				case 0x06: tap(); break;
				case 0x07: tpa(); break;
				case 0x08: inx(); break;
				case 0x09: dex(); break;
				case 0x0A: CLV; break;
				case 0x0B: SEV; break;
				case 0x0C: CLC; break;
				case 0x0D: SEC; break;
				case 0x0E: cli(); break;
				case 0x0F: sei(); break;
				case 0x10: sba(); break;
				case 0x11: cba(); break;
				case 0x12: illegal(); break;
				case 0x13: illegal(); break;
				case 0x14: illegal(); break;
				case 0x15: illegal(); break;
				case 0x16: tab(); break;
				case 0x17: tba(); break;
				case 0x18: illegal(); break;
				case 0x19: daa(); break;
				case 0x1a: illegal(); break;
				case 0x1b: aba(); break;
				case 0x1c: illegal(); break;
				case 0x1d: illegal(); break;
				case 0x1e: illegal(); break;
				case 0x1f: illegal(); break;
				case 0x20: bra(); break;
				case 0x21: brn(); break;
				case 0x22: bhi(); break;
				case 0x23: bls(); break;
				case 0x24: bcc(); break;
				case 0x25: bcs(); break;
				case 0x26: bne(); break;
				case 0x27: beq(); break;
				case 0x28: bvc(); break;
				case 0x29: bvs(); break;
				case 0x2a: bpl(); break;
				case 0x2b: bmi(); break;
				case 0x2c: bge(); break;
				case 0x2d: blt(); break;
				case 0x2e: bgt(); break;
				case 0x2f: ble(); break;
				case 0x30: tsx(); break;
				case 0x31: ins(); break;
				case 0x32: pula(); break;
				case 0x33: pulb(); break;
				case 0x34: des(); break;
				case 0x35: txs(); break;
				case 0x36: psha(); break;
				case 0x37: pshb(); break;
				case 0x38: illegal(); break;
				case 0x39: rts(); break;
				case 0x3a: illegal(); break;
				case 0x3b: rti(); break;
				case 0x3c: illegal(); break;
				case 0x3d: illegal(); break;
				case 0x3e: wai(); break;
				case 0x3f: swi(); break;
				case 0x40: nega(); break;
				case 0x41: illegal(); break;
				case 0x42: illegal(); break;
				case 0x43: coma(); break;
				case 0x44: lsra(); break;
				case 0x45: illegal(); break;
				case 0x46: rora(); break;
				case 0x47: asra(); break;
				case 0x48: asla(); break;
				case 0x49: rola(); break;
				case 0x4a: deca(); break;
				case 0x4b: illegal(); break;
				case 0x4c: inca(); break;
				case 0x4d: tsta(); break;
				case 0x4e: illegal(); break;
				case 0x4f: clra(); break;
				case 0x50: negb(); break;
				case 0x51: illegal(); break;
				case 0x52: illegal(); break;
				case 0x53: comb(); break;
				case 0x54: lsrb(); break;
				case 0x55: illegal(); break;
				case 0x56: rorb(); break;
				case 0x57: asrb(); break;
				case 0x58: aslb(); break;
				case 0x59: rolb(); break;
				case 0x5a: decb(); break;
				case 0x5b: illegal(); break;
				case 0x5c: incb(); break;
				case 0x5d: tstb(); break;
				case 0x5e: illegal(); break;
				case 0x5f: clrb(); break;
				case 0x60: neg_ix(); break;
				case 0x61: illegal(); break;
				case 0x62: illegal(); break;
				case 0x63: com_ix(); break;
				case 0x64: lsr_ix(); break;
				case 0x65: illegal(); break;
				case 0x66: ror_ix(); break;
				case 0x67: asr_ix(); break;
				case 0x68: asl_ix(); break;
				case 0x69: rol_ix(); break;
				case 0x6a: dec_ix(); break;
				case 0x6b: illegal(); break;
				case 0x6c: inc_ix(); break;
				case 0x6d: tst_ix(); break;
				case 0x6e: jmp_ix(); break;
				case 0x6f: clr_ix(); break;
				case 0x70: neg_ex(); break;
				case 0x71: illegal(); break;
				case 0x72: illegal(); break;
				case 0x73: com_ex(); break;
				case 0x74: lsr_ex(); break;
				case 0x75: illegal(); break;
				case 0x76: ror_ex(); break;
				case 0x77: asr_ex(); break;
				case 0x78: asl_ex(); break;
				case 0x79: rol_ex(); break;
				case 0x7a: dec_ex(); break;
				case 0x7b: illegal(); break;
				case 0x7c: inc_ex(); break;
				case 0x7d: tst_ex(); break;
				case 0x7e: jmp_ex(); break;
				case 0x7f: clr_ex(); break;
				case 0x80: suba_im(); break;
				case 0x81: cmpa_im(); break;
				case 0x82: sbca_im(); break;
				case 0x83: illegal(); break;
				case 0x84: anda_im(); break;
				case 0x85: bita_im(); break;
				case 0x86: lda_im(); break;
				case 0x87: sta_im(); break;
				case 0x88: eora_im(); break;
				case 0x89: adca_im(); break;
				case 0x8a: ora_im(); break;
				case 0x8b: adda_im(); break;
				case 0x8c: cmpx_im(); break;
				case 0x8d: bsr(); break;
				case 0x8e: lds_im(); break;
				case 0x8f: sts_im(); /* orthogonality */ break;
				case 0x90: suba_di(); break;
				case 0x91: cmpa_di(); break;
				case 0x92: sbca_di(); break;
				case 0x93: illegal(); break;
				case 0x94: anda_di(); break;
				case 0x95: bita_di(); break;
				case 0x96: lda_di(); break;
				case 0x97: sta_di(); break;
				case 0x98: eora_di(); break;
				case 0x99: adca_di(); break;
				case 0x9a: ora_di(); break;
				case 0x9b: adda_di(); break;
				case 0x9c: cmpx_di(); break;
				case 0x9d: jsr_di(); break;
				case 0x9e: lds_di(); break;
				case 0x9f: sts_di(); break;
				case 0xa0: suba_ix(); break;
				case 0xa1: cmpa_ix(); break;
				case 0xa2: sbca_ix(); break;
				case 0xa3: illegal(); break;
				case 0xa4: anda_ix(); break;
				case 0xa5: bita_ix(); break;
				case 0xa6: lda_ix(); break;
				case 0xa7: sta_ix(); break;
				case 0xa8: eora_ix(); break;
				case 0xa9: adca_ix(); break;
				case 0xaa: ora_ix(); break;
				case 0xab: adda_ix(); break;
				case 0xac: cmpx_ix(); break;
				case 0xad: jsr_ix(); break;
				case 0xae: lds_ix(); break;
				case 0xaf: sts_ix(); break;
				case 0xb0: suba_ex(); break;
				case 0xb1: cmpa_ex(); break;
				case 0xb2: sbca_ex(); break;
				case 0xb3: illegal(); break;
				case 0xb4: anda_ex(); break;
				case 0xb5: bita_ex(); break;
				case 0xb6: lda_ex(); break;
				case 0xb7: sta_ex(); break;
				case 0xb8: eora_ex(); break;
				case 0xb9: adca_ex(); break;
				case 0xba: ora_ex(); break;
				case 0xbb: adda_ex(); break;
				case 0xbc: cmpx_ex(); break;
				case 0xbd: jsr_ex(); break;
				case 0xbe: lds_ex(); break;
				case 0xbf: sts_ex(); break;
				case 0xc0: subb_im(); break;
				case 0xc1: cmpb_im(); break;
				case 0xc2: sbcb_im(); break;
				case 0xc3: illegal(); break;
				case 0xc4: andb_im(); break;
				case 0xc5: bitb_im(); break;
				case 0xc6: ldb_im(); break;
				case 0xc7: stb_im(); break;
				case 0xc8: eorb_im(); break;
				case 0xc9: adcb_im(); break;
				case 0xca: orb_im(); break;
				case 0xcb: addb_im(); break;
				case 0xcc: illegal(); break;
				case 0xcd: illegal(); break;
				case 0xce: ldx_im(); break;
				case 0xcf: stx_im(); break;
				case 0xd0: subb_di(); break;
				case 0xd1: cmpb_di(); break;
				case 0xd2: sbcb_di(); break;
				case 0xd3: illegal(); break;
				case 0xd4: andb_di(); break;
				case 0xd5: bitb_di(); break;
				case 0xd6: ldb_di(); break;
				case 0xd7: stb_di(); break;
				case 0xd8: eorb_di(); break;
				case 0xd9: adcb_di(); break;
				case 0xda: orb_di(); break;
				case 0xdb: addb_di(); break;
				case 0xdc: illegal(); break;
				case 0xdd: illegal(); break;
				case 0xde: ldx_di(); break;
				case 0xdf: stx_di(); break;
				case 0xe0: subb_ix(); break;
				case 0xe1: cmpb_ix(); break;
				case 0xe2: sbcb_ix(); break;
				case 0xe3: illegal(); break;
				case 0xe4: andb_ix(); break;
				case 0xe5: bitb_ix(); break;
				case 0xe6: ldb_ix(); break;
				case 0xe7: stb_ix(); break;
				case 0xe8: eorb_ix(); break;
				case 0xe9: adcb_ix(); break;
				case 0xea: orb_ix(); break;
				case 0xeb: addb_ix(); break;
				case 0xec: illegal(); break;
				case 0xed: illegal(); break;
				case 0xee: ldx_ix(); break;
				case 0xef: stx_ix(); break;
				case 0xf0: subb_ex(); break;
				case 0xf1: cmpb_ex(); break;
				case 0xf2: sbcb_ex(); break;
				case 0xf3: illegal(); break;
				case 0xf4: andb_ex(); break;
				case 0xf5: bitb_ex(); break;
				case 0xf6: ldb_ex(); break;
				case 0xf7: stb_ex(); break;
				case 0xf8: eorb_ex(); break;
				case 0xf9: adcb_ex(); break;
				case 0xfa: orb_ex(); break;
				case 0xfb: addb_ex(); break;
				case 0xfc: addx_ex(); break;
				case 0xfd: illegal(); break;
				case 0xfe: ldx_ex(); break;
				case 0xff: stx_ex(); break;
			}
		}
	} while( m6800_ICount>0 );
	m6800.extra_cycles = 0;

	return cycles - m6800_ICount;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *m6800_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	m6800_Regs const *r = static_cast<m6800_Regs const *>(context);

	which = (which+1) % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &m6800;

	switch( regnum )
	{
		case M6800_A: sprintf(buffer[which], "A:%02X", r->d.b.h); break;
		case M6800_B: sprintf(buffer[which], "B:%02X", r->d.b.l); break;
		case M6800_PC: sprintf(buffer[which], "PC:%04X", r->pc.w.l); break;
		case M6800_S: sprintf(buffer[which], "S:%04X", r->s.w.l); break;
		case M6800_X: sprintf(buffer[which], "X:%04X", r->x.w.l); break;
		case M6800_CC: sprintf(buffer[which], "CC:%02X", r->cc); break;
		case M6800_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
		case M6800_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state); break;
	}
	return buffer[which];
}

unsigned m6800_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm680x(6800,buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}


/****************************************************************************
 * M6808 almost (fully?) equal to the M6800
 ****************************************************************************/
void m6808_init(void)
{
	m6800.insn = m6800_insn;
	m6800.cycles = cycles_6800;
}

void m6808_reset(void *param) { m6800_reset(param); }
void m6808_exit(void) { m6800_exit(); }
int  m6808_execute(int cycles) { return m6800_execute(cycles); }
unsigned m6808_get_context(void *dst) { return m6800_get_context(dst); }
void m6808_set_context(void *src) { m6800_set_context(src); }
unsigned m6808_get_reg(int regnum) { return m6800_get_reg(regnum); }
void m6808_set_reg(int regnum, unsigned val) { m6800_set_reg(regnum,val); }
void m6808_set_irq_line(int irqline, int state) { m6800_set_irq_line(irqline,state); }
void m6808_set_irq_callback(int (*callback)(int irqline)) { m6800_set_irq_callback(callback); }

unsigned m6808_dasm(char *buffer, unsigned pc)
{
	return Dasm680x(6808,buffer,pc);
	sprintf( buffer, "$%02X", cpu_readmem16(pc) );
	return 1;
}
