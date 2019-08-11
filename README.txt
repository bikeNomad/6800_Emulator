Using NXP's MCUXpresso IDE
project ported from KDS?

Added SEGGER's RTT support

interesting variables:

m6800 (static struct in m6800.cpp)
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

	int	    (*irq_callback)(int irqline);
	int 	extra_cycles;	/* cycles used for interrupts */
	void	(* const * insn)(void);	/* instruction table */
	const UINT8 *cycles;			/* clock cycle of instruction table */
	unsigned wait_cycles;	/* counting total wait cycles */
}  m6800_Regs;

has serial shell too

    cs start end: print checksum of ROM
    ex nInstructions: execute
    hd addr count: hex dump
    led nTimes: pulse diagnostic LED
    wm addr value: write memory
