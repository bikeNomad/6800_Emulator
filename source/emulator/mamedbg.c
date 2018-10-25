/****************************************************************************
 *	MAME debugger V0.54
 *	Juergen Buchmueller <pullmoll@t-online.de>
 *
 *	Based on code found in the preivous version of the MAME debugger
 *	written by: Martin Scragg, John Butler, Mirko Buffoni
 *	Chris Moore, Aaron Giles, Ernesto Corvi
 *
 *	Online help is available by pressing F1 (context sensitive!)
 *
 *	TODO:
 *	- Add stack view using activecpu_get_reg(REG_SP_CONTENTS+offset)
 *	- Add more display modes for the memory windows (binary? octal? decimal?)
 *
 ****************************************************************************/

#include <stdio.h>

#ifdef MAME_DEBUG
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "osd_cpu.h"
// #include "driver.h"
// #include "vidhrdw/generic.h"
#include "cpuintrf.h"
#include "mamedbg.h"
// #include "window.h"

#define MAX_CPU 1

#ifndef INVALID
#define INVALID -1
#endif

/****************************************************************************
 * Externals (define in the header files)
 ****************************************************************************/

#define RDMEM(a)	(*cputype_get_interface(cputype)->memory_read)(a)
#define WRMEM(a,v)	(*cputype_get_interface(cputype)->memory_write)(a,v)
#define RDINT(a)	(*cputype_get_interface(cputype)->internal_read)(a)
#define WRINT(a,v)	(*cputype_get_interface(cputype)->internal_write)(a,v)
#define PGM_MEMORY	cputype_get_interface(cputype)->pgm_memory_base

/****************************************************************************
 * Globals
 ****************************************************************************/


/****************************************************************************
 * Limits
 ****************************************************************************/
#define MAX_DATA	512 		/* Maximum memory size in bytes of a dump window */
#define MAX_MEM 	2			/* You can't redefine this... too easy */

#define MAX_LOOPS	64			/* Maximum loop addresses recognized by trace */

#define MAX_HIST	16			/* Maximum history depth */

#define EDIT_CMDS	0
#define EDIT_REGS	1			/* Just the order of the windows */
#define EDIT_DASM	2
#define EDIT_MEM1	3
#define EDIT_MEM2	4

#define DBG_WINDOWS 5

/* Some convenience macros to address the cpu'th window */
#define WIN_CMDS(cpu)	((cpu)*DBG_WINDOWS+EDIT_CMDS)
#define WIN_REGS(cpu)	((cpu)*DBG_WINDOWS+EDIT_REGS)
#define WIN_DASM(cpu)	((cpu)*DBG_WINDOWS+EDIT_DASM)
#define WIN_MEM(cpu,n)	((cpu)*DBG_WINDOWS+EDIT_MEM1+(n))
#define WIN_MEM1(cpu)	((cpu)*DBG_WINDOWS+EDIT_MEM1)
#define WIN_MEM2(cpu)	((cpu)*DBG_WINDOWS+EDIT_MEM2)
#define WIN_HELP		(MAX_WINDOWS-1)
#define WIN_MSGBOX		(MAX_WINDOWS-2)

enum {
	MODE_HEX_UINT8,
	MODE_HEX_UINT16,
	MODE_HEX_UINT32,
	MODE_HEX_COUNT
};

enum {
	MODE_CHR_HEX,
	MODE_CHR_XLATE,
	MODE_CHR_PLAIN,
	MODE_CHR_COUNT
};

#define UINT16_XOR_LE(o) (((o)&~1)|(((o)&1)^1))
#define UINT32_XOR_LE(o) (((o)&~3)|(((o)&3)^3))
#define HOST_XOR_LE	0
#define UINT16_XOR_BE(o) (o)
#define UINT32_XOR_BE(o) (o)
#define HOST_XOR_BE	0

/****************************************************************************
 * Statics
 ****************************************************************************/
static int first_time = 1;

static int active_cpu = INVALID;
static int previous_active_cpu = INVALID;
static int total_cpu = 0;
static int cputype = 0;

static int dbg_fast = 0;
static int dbg_step = 0;
static int dbg_trace = 0;
static int dbg_update = 0;
static int dbg_update_cur = 0;
static int dbg_active = 0;
static int dbg_trace_delay = 0;

/* 0 = dont, 1 = do allow squeezed display w/alternating dim, bright colors */
static int dbg_mem_squeezed = 0;
/* 0 = display disassembly only, 1 = display opcodes too */
static int dbg_dasm_opcodes = 0;
/* 0 = default, 1 = lower or 2 = upper case */
static int dbg_dasm_case = 0;
/* 0 = absolute, 1 = relative format for relative jumps/branches */
static int dbg_dasm_relative_jumps = 0;

static const char *dbg_info_once = NULL;

#ifdef PINMAME
static int dbg_show_scanline = 0;
#else
static int dbg_show_scanline = 1;
#endif /* PINMAME */

/****************************************************************************
 * Color settings
 ****************************************************************************/
#define COLOR_NAMES \
	"BLACK\0" \
	"BLUE\0" \
	"GREEN\0" \
	"CYAN\0" \
	"RED\0" \
	"MAGENTA\0" \
	"BROWN\0" \
	"LIGHTGRAY\0" \
	"DARKGRAY\0" \
	"LIGHTBLUE\0" \
	"LIGHTGREEN\0" \
	"LIGHTCYAN\0" \
	"LIGHTRED\0" \
	"LIGHTMAGENTA\0" \
	"YELLOW\0" \
	"WHITE\0"

#define ELEMENT_NAMES \
	"TITLE\0" \
	"FRAME\0" \
	"REGS\0" \
	"DASM\0" \
	"MEM1\0" \
	"MEM2\0" \
	"CMDS\0" \
	"BRK_EXEC\0" \
	"BRK_DATA\0" \
	"BRK_REGS\0" \
	"ERROR\0" \
	"HELP\0" \
	"PROMPT\0" \
	"CHANGES\0" \
	"PC\0" \
	"CURSOR\0"

enum ELEMENT {
	E_TITLE,
	E_FRAME,
	E_REGS,
	E_DASM,
	E_MEM1,
	E_MEM2,
	E_CMDS,
	E_BRK_EXEC,
	E_BRK_DATA,
	E_BRK_REGS,
	E_ERROR,
	E_HELP,
	E_PROMPT,
	E_CHANGES,
	E_PC,
	E_CURSOR,
	E_COUNT
};

static UINT8 cur_col[E_COUNT] = {
	COLOR_TITLE,
	COLOR_FRAME,
	COLOR_REGS,
	COLOR_DASM,
	COLOR_MEM1,
	COLOR_MEM2,
	COLOR_CMDS,
	COLOR_BRK_EXEC,
	COLOR_BRK_DATA,
	COLOR_BRK_REGS,
	COLOR_ERROR,
	COLOR_HELP,
	COLOR_PROMPT,
	COLOR_CHANGES,
	COLOR_PC,
	COLOR_CURSOR,
};

static UINT8 def_col[E_COUNT] = {
	COLOR_TITLE,
	COLOR_FRAME,
	COLOR_REGS,
	COLOR_DASM,
	COLOR_MEM1,
	COLOR_MEM2,
	COLOR_CMDS,
	COLOR_BRK_EXEC,
	COLOR_BRK_DATA,
	COLOR_BRK_REGS,
	COLOR_ERROR,
	COLOR_HELP,
	COLOR_PROMPT,
	COLOR_CHANGES,
	COLOR_PC,
	COLOR_CURSOR,
};

/****************************************************************************
 * Code to ASCII translation table; may be redefined (later)
 ****************************************************************************/
static char trans_table[256] = {
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	' ','!','"','#','$','%','&', 39,'(',')','*','+',',','-','.','/',
	'0','1','2','3','4','5','6','7','8','9',':',';','<','=','>','?',
	'@','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W','X','Y','Z','[', 92,']','^','_',
	'`','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
	'p','q','r','s','t','u','v','w','x','y','z','{','|','}','~','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
};

/****************************************************************************
 * Function prototypes
 ****************************************************************************/
static unsigned get_register_id( char **parg, int *size );
static const char *get_register_name( int id );
static unsigned get_register_or_value( char **parg, int *size );
static void trace_init( const char *filename, UINT8 *regs );
static void trace_done( void );
static void trace_select( void );
static void trace_output( void );

static int hit_brk_exec( void );
static int hit_brk_data( void );
static int hit_brk_regs( void );

static const char *name_rom( const char *type, int region, unsigned *base, unsigned start );
static const char *name_rdmem( unsigned base );
static const char *name_wrmem( unsigned base );
static const char *name_memory( unsigned base );

static int win_create(int n, UINT8 prio, int x, int y, int w, int h,
	UINT8 co_text, UINT8 co_frame, UINT8 chr, UINT32 attributes);
static int DECL_SPEC win_msgbox( UINT8 color, const char *title, const char *fmt, ... );
static void dbg_open_windows( void );
static void dbg_close_windows( void );

static unsigned dasm_line( unsigned pc, int times );

static void dump_regs( void );
static unsigned dump_dasm( unsigned pc );
static void dump_mem_hex( int which, unsigned len_addr, unsigned len_data );
static void dump_mem( int which, int set_title );

static int edit_cmds_info( void );
static int edit_cmds_parse( char *cmdline );
static void edit_cmds_append( const char *src );

static void edit_regs( void );
static void edit_dasm( void );
static void edit_mem( int which );
static void edit_cmds(void);

static void cmd_help( void );
static void cmd_default( int code );

static void cmd_display_memory( void );
static void cmd_edit_memory( void );
static void cmd_set_memory_mode( void );
static void cmd_fast( void );
static void cmd_go_break( void );
static void cmd_jump( void );
static void cmd_replace_register( void );
static void cmd_brk_exec_set( void );
static void cmd_brk_exec_clear( void );
static void cmd_brk_regs_set( void );
static void cmd_brk_regs_clear( void );
static void cmd_brk_data_set( void );
static void cmd_brk_data_clear( void );
static void cmd_here( void );
static void cmd_dasm_to_file( void );
static void cmd_dump_to_file( void );
static void cmd_trace_to_file( void );
static void cmd_save_to_file( void );
static void cmd_set_ignore( void );
static void cmd_set_observe( void );
static void cmd_set_key_repeat( void );
static void cmd_set_dasm_case( void );
static void cmd_set_dasm_opcodes( void );
static void cmd_set_dasm_relative_jumps( void );
static void cmd_set_mem_squeezed( void );
static void cmd_set_element_color( void );
static void cmd_brk_exec_toggle( void );
static void cmd_brk_data_toggle( void );
static void cmd_toggle_scanlines( void );

static void cmd_switch_window( void );
static void cmd_dasm_up( void );
static void cmd_dasm_down( void );
static void cmd_dasm_page_up( void );
static void cmd_dasm_page_down( void );
static void cmd_dasm_home( void );
static void cmd_dasm_end( void );
static void cmd_dasm_hist_follow( void );
static void cmd_dasm_hist_back( void );
static void cmd_run_to_cursor( void );
static void cmd_focus_next_cpu( void );
static void cmd_step( void );
static void cmd_animate( void );
static void cmd_step_over( void );
static void cmd_go( void );
static void cmd_search_memory( void );
#if defined(PINMAME) && defined(DBG_BPR)
static void bpr_set(void);
void edit_cmds_reset( void );
#endif /* DBG_BPR */

#ifdef PINMAME
static void cmd_jumpover( void );
static void cmd_sound_enable( void );
static void cmd_sound_disable( void );

/* CODELIST */
static void cmd_codelist_to_file( void );
static void codelist_init( const char *filename );
static void codelist_done( void );
static void codelist_select( void );
#endif /* PINMAME */

/****************************************************************************
 * Generic structure for saving points in the 'follow history'
 ****************************************************************************/
typedef struct {
	UINT32	dasm_top;				/* previous top of window PC */
	UINT32	dasm_cur;				/* previous cursor PC */
	UINT32	mem1_base;				/* previous memory 1 base address */
	UINT32	mem1_offset;			/* previous memory 1 offset */
	UINT32	mem1_nibble;			/* previous memory 1 nibble */
}	s_hist;

/****************************************************************************
 * Symbol table entry
 ****************************************************************************/
typedef struct {
	UINT32	value;					/* value of the symbol */
	INT32	access; 				/* access mode (EA_... enum) */
	INT32	size;					/* size of the element */
	UINT32	times;					/* repeat how many times */
	char	name[47+1]; 			/* name of the symbol */
}	s_symbol;

/****************************************************************************
 * Generic structure for editing values
 * x,y are the coordinates inside a window
 * w is the width in hex digits (aka nibbles)
 * n is the distance of the hex part from the start of the output (register)
 ****************************************************************************/
typedef struct {
	UINT8	x,y,w,n;
}	s_edit;

/****************************************************************************
 * Register display and editing structure
 ****************************************************************************/
typedef struct {
	UINT32	backup[MAX_REGS];		/* backup register values */
	UINT32	newval[MAX_REGS];		/* new register values */
	s_edit	edit[MAX_REGS]; 		/* list of x,y,w triplets for the register values */
	char	name[MAX_REGS][15+1];	/* ...fifteen characters enough!? */
	UINT8	id[MAX_REGS];			/* the ID of the register (activecpu_get_reg/activecpu_set_reg) */
	UINT32	max_width;				/* maximum width of any dumped register */
	INT32	idx;					/* index of current register */
	INT32	count;					/* number of registers */
	INT32	nibble; 				/* edit nibble */
	INT32	changed;
	INT32	top;
	INT32	base;
}	s_regs;

/****************************************************************************
 * Memory display and editing structure
 ****************************************************************************/
typedef struct {
	UINT8	backup[MAX_DATA];	/* backup data */
	UINT8	newval[MAX_DATA];	/* newly read data */
	s_edit	edit[MAX_DATA]; 	/* list of x,y,w triplets for the memory elements */
	UINT32	base;				/* current base address */
	UINT32	address;			/* current cursor address */
	UINT32	pgm_memory_base;	/* program/data memory base toggle */
	INT32	offset; 			/* edit offset */
	INT32	nibble; 			/* edit nibble */
	INT32	bytes;				/* number of bytes per edit line */
	INT32	width;				/* width in nibbles of the edit line */
	INT32	size;				/* number of bytes in the edit window */
	UINT8	mode;				/* 0 bytes, 1 words, 2 dword */
	UINT8	ascii;				/* display ASCII values */
	UINT8	internal;			/* display CPU internal memory instead of ROM/RAM? */
	UINT8	changed;
}	s_mem;

/****************************************************************************
 * Disassembly structure
 ****************************************************************************/
typedef struct {
	UINT32	pc_cpu; 			/* The CPUs PC */
	UINT32	pc_top; 			/* Top of the disassembly window PC */
	UINT32	pc_cur; 			/* Cursor PC */
	UINT32	pc_end; 			/* End of the disassembly window PC */
	UINT32	dst_ea_value;		/* effective destination address or value */
	INT32	dst_ea_access;		/* destination access mode */
	INT32	dst_ea_size;		/* destination access size */
	UINT32	src_ea_value;		/* effective source address or value */
	INT32	src_ea_access;		/* source access mode */
	INT32	src_ea_size;		/* source access size */
}	s_dasm;

/****************************************************************************
 * Tracing structure
 ****************************************************************************/
#ifdef PINMAME
/* CODELIST */
struct s_codeblock {
	/* address data */
	UINT32	startAddr;
	UINT32	afterAddr;

	/* linkage */
	struct s_codeblock	*next;
};

struct s_codelist {
	/* code data */
	UINT32	bankid;
	struct s_codeblock	*blocks;

	/* linkage */
	struct s_codelist	*next;
};
#endif /* PINMAME */

typedef struct {
	UINT32	last_pc[MAX_LOOPS];
	UINT8	regs[MAX_REGS];
	FILE	*file;
	INT32	iters;
	INT32	loops;

#ifdef PINMAME
/* CODELIST */
	FILE	*codelistfile;
	struct s_codelist	*codelist;
	unsigned addr_width;
	unsigned addr_mask;
#endif /* PINMAME */
}	s_trace;

/****************************************************************************
 * Debugger structure. There is one instance per CPU
 ****************************************************************************/
typedef struct {
	UINT32	ignore; 			/* ignore this CPU while debugging? */
	UINT32	next_pc;
	UINT32	prev_sp;

	/* Break- and Watchpoints */
	UINT32	brk_exec;			/* execution breakpoint (program counter) */
	UINT32	brk_exec_times; 	/* how many times to ignore the breakpoint */
	UINT32	brk_exec_reset; 	/* reset value for times once it counted down */
	UINT32	brk_data;			/* data watchpoint (memory address) */
	UINT32	brk_data_oldval;	/* old data watchpoint value */
	UINT32	brk_data_newval;	/* expected new value (INVALID: always break) */
	UINT32	brk_regs;			/* register watchpoint (register ID) */
	UINT32	brk_regs_oldval;	/* old register watchpoint value */
	UINT32	brk_regs_newval;	/* expected new value (INVALID: always break) */
	UINT32	brk_regs_mask;		/* mask register value before comparing */
	UINT32	brk_temp;			/* temporary execution breakpoint */
#if defined(PINMAME) && defined(DBG_BPR)
	UINT32	bprstart, bprend;
	int		bprnext;
	struct { UINT32 adr; int length; } bprdata[10];
#endif

	s_regs	regs;
	s_dasm	dasm;
	s_mem	mem[MAX_MEM];
	s_trace trace;
	INT32	hist_cnt;
	s_hist	hist[MAX_HIST];

	char	cmdline[80+1];
	UINT8	window; 	/* edit what: cmds, regs, dasm, mem1, mem2 */

}	s_dbg;

static	s_dbg	dbg[MAX_CPU];

/* Covenience macros... keep the code readable */
#define DBG 	dbg[active_cpu]
#define DBGREGS dbg[active_cpu].regs
#define DBGDASM dbg[active_cpu].dasm
#define DBGMEM	dbg[active_cpu].mem
#define TRACE	dbg[tracecpu].trace

#define CMD 	dbg[active_cpu].cmdline

UINT8 debugger_idle;

int stringcasecmp(const char *s1, const char *s2) {
	while (*s1 != 0 && tolower(*s1) == tolower(*s2)) {
		++s1;
		++s2;
	}

	return
		(*s2 == 0)
		? (*s1 != 0)
		: (*s1 == 0)
		? -1
		: (tolower(*s1) - tolower(*s2));
}


/****************************************************************************
 * Tracing
 ****************************************************************************/
static int tracecpu = 0;
static int trace_on = 0;

#ifdef PINMAME
/* CODELIST */
static int codelist_on = 0;
#endif /* PINMAME */

/****************************************************************************
 * Commands structure
 ****************************************************************************/
typedef struct {
	int valid;				/* command is valid for which windows (bit mask) */
	const char *name;		/* command name (NULL none) */
	const char *alias;		/* command name alias (NULL none) */
	int key;				/* key code (0 none) */
	const char *args;		/* description of expected arguments */
	const char *info;		/* description of the function */
	void (*function)(void); /* function handling the key/command */
}	s_command;

#define ALL 	((1<<EDIT_CMDS)|(1<<EDIT_REGS)|(1<<EDIT_DASM)|(1<<EDIT_MEM1)|(1<<EDIT_MEM2))

INLINE unsigned order( unsigned offset, unsigned size )
{
	switch( size )
	{
	case 1:
		return offset;
		break;
	case 2:
		switch( ENDIAN )
		{
		case CPU_IS_LE: return UINT16_XOR_LE(offset);
		case CPU_IS_BE: return UINT16_XOR_BE(offset);
		}
		break;
	case 4:
		switch( ENDIAN )
		{
		case CPU_IS_LE: return UINT32_XOR_LE(offset);
		case CPU_IS_BE: return UINT32_XOR_BE(offset);
		}
		break;
	}
	return offset;
}

/* adjust an offset by shifting it left activecpu_address_shift() times */
INLINE unsigned lshift( unsigned offset )
{
	int shift = ASHIFT;
	if (shift > 0)
		offset <<= shift;
	else
		offset >>= -shift;
	return offset;
}

/* adjust an offset by shifting it right activecpu_address_shift() times */
INLINE unsigned rshift( unsigned offset )
{
	int shift = ASHIFT;
	if (shift > 0)
		offset >>= shift;
	else
		offset <<= -shift;
	return offset;
}


/**************************************************************************
 * dtou
 * Decimal to unsigned.
 * The pointer to the char* is placed after all consecutive digits
 * and trailing space. The pointer to int size (if given) contains the
 * number of digits found.
 **************************************************************************/
INLINE unsigned dtou( char **parg, int *size)
{
	unsigned val = 0, digit;

	if (size) *size = 0;
	while( isdigit( *(*parg) ) )
	{
		digit = *(*parg) - '0';
		val = (val * 10) + digit;
		if( size ) (*size)++;
		(*parg) += 1;
	}
	while( isspace(*(*parg)) ) *parg += 1;
	return val;
}

/**************************************************************************
 * xtou
 * Hex to unsigned.
 * The pointer to the char* is placed after all consecutive hex digits
 * and trailing space. The pointer to int size (if given) contains the
 * number of digits found.
 **************************************************************************/
INLINE unsigned xtou( char **parg, int *size)
{
	unsigned val = 0, digit;

	if (size) *size = 0;
	while( isxdigit( *(*parg) ) )
	{
		digit = toupper(*(*parg)) - '0';
		if( digit > 9 ) digit -= 7;
		val = (val << 4) | digit;
		if( size ) (*size)++;
		(*parg) += 1;
	}
	while( isspace(*(*parg)) ) *parg += 1;
	return val;
}

const char *set_ea_info( int what, unsigned value, int size, int access )
{
	static char buffer[8][63+1];
	static int which = 0;
	const char *sign = "";
	unsigned width, result;

	which = (which+1) % 8;

	if( access == EA_REL_PC )
		/* PC relative calls set_ea_info with value = PC and size = offset */
		result = value + size;
	else
		result = value;

	/* set source EA? */
	if( what == EA_SRC )
	{
		DBGDASM.src_ea_access = access;
		DBGDASM.src_ea_value = result;
		DBGDASM.src_ea_size = size;
	}
	else
	if( what == EA_DST )
	{
		DBGDASM.dst_ea_access = access;
		DBGDASM.dst_ea_value = result;
		DBGDASM.dst_ea_size = size;
	}
	else
	{
		return "set_ea_info: invalid <what>!";
	}

	switch( access )
	{
	case EA_VALUE:	/* Immediate value */
		switch( size )
		{
		case EA_INT8:
		case EA_UINT8:
			width = 2;
			break;
		case EA_INT16:
		case EA_UINT16:
			width = 4;
			break;
		case EA_INT32:
		case EA_UINT32:
			width = 8;
			break;
		default:
			return "set_ea_info: invalid <size>!";
		}
		switch( size )
		{
		case EA_INT8:
		case EA_INT16:
		case EA_INT32:
			if( result & (1 << ((width * 4) - 1)) )
			{
				sign = "-";
				result = (unsigned)-result;
			}
			break;
		}

		if (width < 8)
			result &= (1 << (width * 4)) - 1;
		break;

	case EA_ZPG_RD:
	case EA_ZPG_WR:
	case EA_ZPG_RDWR:
		result &= 0xff;
		width = (ABITS + 3) / 4;	// display zero page addresses in their correct length
		break;

	case EA_ABS_PC: /* Absolute program counter change */
		result &= AMASK;
		if( size == EA_INT8 || size == EA_UINT8 )
			width = 2;
		else
		if( size == EA_INT16 || size == EA_UINT16 )
			width = 4;
		else
		if( size == EA_INT32 || size == EA_UINT32 )
			width = 8;
		else
			width = (ABITS + 3) / 4;
		break;

	case EA_REL_PC: /* Relative program counter change */
		if( dbg_dasm_relative_jumps )
		{
			if( size == 0 )
				return "$";
			if( size < 0 )
			{
				sign = "-";
				result = (unsigned) -size;
			}
			else
			{
				sign = "+";
				result = (unsigned) size;
			}
			sprintf( buffer[which], "%s%u", sign, result );
			return buffer[which];
		}
		/* fall through */
	default:
		result &= AMASK;
		width = (ABITS + 3) / 4;
	}
	sprintf( buffer[which], "%s$%0*X", sign, width, result );
	return buffer[which];
}

/**************************************************************************
 * lower
 * Convert string into all lower case.
 **************************************************************************/
INLINE char *lower( const char *src)
{
	static char buffer[127+1];
	char *dst = buffer;
	while( *src )
		*dst++ = tolower(*src++);
	*dst = '\0';
	return buffer;
}

/**************************************************************************
 * upper
 * Convert string into all upper case.
 **************************************************************************/
INLINE char *upper( const char *src)
{
	static char buffer[127+1];
	char *dst = buffer;
	while( *src )
		*dst++ = toupper(*src++);
	*dst = '\0';
	return buffer;
}

/**************************************************************************
 * kilobyte
 * Format a byte count or size to a kilo or mega bytes string
 **************************************************************************/
INLINE char *kilobyte( unsigned bytes )
{
	static char buffer[2][31+1];
	static int which = 0;
	char *dst = buffer[which];
	which ^= 1;
	if( bytes < 1024 )
		sprintf( dst, "%u", bytes );
	else
	if( bytes < 1024 * 1024 )
		sprintf( dst, "%u.%02uK", bytes / 1024, 100 * bytes / 1024 );
	else
		sprintf( dst, "%u.%02uM", bytes / 1024 / 1024, 100 * ((bytes / 1024) % 1024) / 1024 );
	return dst;
}

/**************************************************************************
 * my_stricmp
 * Compare strings case insensitive
 **************************************************************************/
INLINE int my_stricmp( const char *dst, const char *src)
{
	while( *src && *dst )
	{
		if( tolower(*src) != tolower(*dst) ) return *dst - *src;
		src++;
		dst++;
	}
	return *dst - *src;
}

/**************************************************************************
 * get_boolean
 * Get a boolean argument (on/off, yes/no, y/n, 1/0)
 **************************************************************************/
static unsigned get_boolean( char **parg, int *size )
{
	char *p = *parg;
	unsigned result = 0;
	int length = 0;

	if( toupper(p[0]) == 'O' )
	{
		if( toupper(p[1]) == 'N' && !isalnum(p[2]) )
		{
			result = 1;
			length = 2;
			*parg += length;
		}
		else
		if( toupper(p[1]) == 'F' && toupper(p[2]) == 'F' && !isalnum(p[3]) )
		{
			result = 0;
			length = 3;
			*parg += length;
		}
	}
	else
	if( toupper(p[0]) == 'N' )
	{
		if( toupper(p[1]) == 'O' && !isalnum(p[2]) )
		{
			result = 0;
			length = 2;
			*parg += length;
		}
		else
		if( !isalnum(p[1]) )
		{
			result = 0;
			length = 1;
			*parg += length;
		}
	}
	else
	if( toupper(p[0]) == 'Y' )
	{
		if( toupper(p[1]) == 'E' && toupper(p[2]) == 'S' && !isalnum(p[3]) )
		{
			result = 1;
			length = 3;
			*parg += length;
		}
		else
		if( !isalnum(p[1]) )
		{
			result = 1;
			length = 1;
			*parg += length;
		}
	}

	if( length )
	{
		while( isspace(*(*parg)) ) *parg += 1;
	}
	else
	{
		/* found nothing yet: assume numeric */
		result = xtou( parg, &length );
	}

	if( size ) *size += length;

	return result;
}

/**************************************************************************
 * get_option_or_value
 * Get a option argument (from opt_list) or a number in the
 * range of 0 .. number of options - 1
 **************************************************************************/
static unsigned get_option_or_value( char **parg, int *size, const char *opt_list )
{
	char *p = *parg;
	const char *opt;
	unsigned result = 0, opt_count = 0;
	int length = 0;

	/* length of the next argument */
	while( isalnum(*p) ) p++;
	length = (int) (p - *parg);
	while( isspace(*p) ) p++;

	/* sacn options */
	for( opt = opt_list; *opt ; opt += strlen(opt) + 1 )
	{
		if( strncmp(*parg, opt, length) == 0 )
		{
			*parg = p;
			if( size ) *size = length;
			return opt_count;
		}
		opt_count++;
	}

	result = xtou( parg, &length );
	if( size ) *size += length;

	return result;
}

static const char *get_file_name( char **parg, int *size )
{
	static char filename[127+1];
	char *s, *d;
	int l;

	for( l = 0, s = *parg, d = filename; *s && (isalnum(*s) || ispunct(*s)); l++ )
		*d++ = *s++;

	*d = '\0';
	while( isspace(*s) ) s++;
	*parg = s;

	if( size ) *size = l;

	return filename;
}

const char *get_ea_info( unsigned pc )
{
	static char buffer[63+1];
	static const char *access[EA_COUNT] =
	{
		"",     /* no EA mode */
		"#",    /* immediate */
		"=",    /* absolute PC */
		"$",    /* relative PC */
		"<",    /* zero page memory read */
		">",    /* zero page memory write */
		"*",    /* zero page memory modify */
		"<",    /* memory read */
		">",    /* memory write */
		"*",    /* memory modify */
		"P<",   /* port read */
		"P>"    /* port write */
	};

	unsigned wdst, wsrc;

	switch( DBGDASM.dst_ea_size )
	{
	case EA_INT8:	wdst = 2; break;
	case EA_INT16:	wdst = 4; break;
	case EA_INT32:	wdst = 8; break;
	case EA_UINT8:	wdst = 2; break;
	case EA_UINT16: wdst = 4; break;
	case EA_UINT32: wdst = 8; break;
	default:
		wdst = (ABITS + 3) / 4;
	}

	switch( DBGDASM.src_ea_size )
	{
	case EA_INT8:	wsrc = 2; break;
	case EA_INT16:	wsrc = 4; break;
	case EA_INT32:	wsrc = 8; break;
	case EA_UINT8:	wsrc = 2; break;
	case EA_UINT16: wsrc = 4; break;
	case EA_UINT32: wsrc = 8; break;
	default:
		wsrc = (ABITS + 3) / 4;
	}

	if( DBGDASM.dst_ea_value != INVALID && DBGDASM.src_ea_value != INVALID )
		sprintf( buffer, "%s\t%s%0*X %s%0*X",
			name_rdmem(rshift(pc)),
			access[DBGDASM.src_ea_access], wsrc, DBGDASM.src_ea_value,
			access[DBGDASM.dst_ea_access], wdst, DBGDASM.dst_ea_value );
	else
	if( DBGDASM.dst_ea_value != INVALID )
		sprintf( buffer, "%s\t%s%0*X",
			name_rdmem(rshift(pc)),
			access[DBGDASM.dst_ea_access], wdst, DBGDASM.dst_ea_value );
	else
	if( DBGDASM.src_ea_value != INVALID )
		sprintf( buffer, "%s\t%s%0*X",
			name_rdmem(rshift(pc)),
			access[DBGDASM.src_ea_access], wsrc, DBGDASM.src_ea_value );
	else
		sprintf( buffer, "%s", name_rdmem(rshift(pc)) );

	return buffer;
}

/**************************************************************************
 * get_register_id
 * Return the ID for a register if the string at *parg matches one
 * of the register names for the active cpu.
 **************************************************************************/
static unsigned get_register_id( char **parg, int *size )
{
	int i, l;
	for( i = 0; i < DBGREGS.count; i++ )
	{
		l = strlen( DBGREGS.name[i] );
		if( l > 0 && !stringcasecmp( *parg, DBGREGS.name[i] ) )
		{
			if( !isalnum( (*parg)[l] ) )
			{
				if( size ) *size = l;
				*parg += l;
				while( isspace(*(*parg)) ) *parg += 1;
				return DBGREGS.id[i];
			}
		}
	}
	if( size ) *size = 0;
	return 0;
}

/**************************************************************************
 * get_register_name
 * Get the name of a register with ID
 **************************************************************************/
static const char *get_register_name( int id )
{
	int i;
	for( i = 0; i < DBGREGS.count; i++ )
	{
		if( DBGREGS.id[i] == id )
			return DBGREGS.name[i];
	}
	return "??";
}

/**************************************************************************
 * get_register_or_value
 * Return the value for a register if the string at *parg matches one
 * of the register names for the active cpu. Otherwise get a hex
 * value from *parg. In both cases set the pointer int size to the
 * length of the name or digits found (if size is not NULL)
 **************************************************************************/
static unsigned get_register_or_value( char **parg, int *size )
{
	int regnum, l;

	regnum = get_register_id( parg, &l );
	if( regnum > 0 )
	{
		if( size ) *size = l;
		return activecpu_get_reg( regnum );
	}
	/* default to hex value */
	return xtou( parg, size );
}

/**************************************************************************
 * trace_init
 * Creates trace output files for all CPUs
 * Resets the loop and iteration counters and the last PC array
 **************************************************************************/
static void trace_init( const char *filename, UINT8 *regs )
{
	char name[100];

	if( trace_on )
		return;

	for( tracecpu = 0; tracecpu < total_cpu; tracecpu++ )
	{
		sprintf( name, "%s.%d", filename, tracecpu );
		TRACE.file = fopen(name,"w");
		if( tracecpu == active_cpu )
			memcpy( TRACE.regs, regs, MAX_REGS );
		else
			TRACE.regs[0] = 0;
		TRACE.iters = 0;
		TRACE.loops = 0;
		memset(TRACE.last_pc, 0xff, sizeof(TRACE.last_pc));
	}
	tracecpu = active_cpu;
	trace_on = 1;
}

/**************************************************************************
 * trace_done
 * Closes the trace output files
 **************************************************************************/
void trace_done(void)
{
	if( !trace_on )
		return;

	for( tracecpu = 0; tracecpu < total_cpu; tracecpu++ )
	{
		if( TRACE.file )
			fclose( TRACE.file );
		TRACE.file = NULL;
	}

	trace_on = 0;
}

/**************************************************************************
 * trace_select
 * Switches tracing to the active CPU
 **************************************************************************/
static void trace_select( void )
{
	if( tracecpu == active_cpu )
		return;
	if( trace_on && TRACE.file )
	{
		if( TRACE.loops )
		{
			fprintf( TRACE.file,
				"\n   (loops for %d instructions)\n\n",
				TRACE.loops );
			TRACE.loops = 0;
		}
		fprintf(TRACE.file,"\n=============== End of iteration #%d ===============\n\n",TRACE.iters++);
		fflush(TRACE.file);
	}
	if( active_cpu < total_cpu )
		tracecpu = active_cpu;
}

/**************************************************************************
 * trace_output
 * Outputs the next disassembled instruction to the trace file
 * Loops are detected and a loop count is output after the
 * first repetition instead of disassembling the loop over and over
 **************************************************************************/
static void trace_output( void )
{
	static char buffer[127+1];
	char *dst = buffer;

	if( trace_on && TRACE.file )
	{
		unsigned pc = activecpu_get_pc();
		unsigned addr_width = (ABITS + 3) / 4;
		int count, i;

		/* check for trace_loops */
		for( i = count = 0; i < MAX_LOOPS; i++ )
			if( TRACE.last_pc[i] == pc )
				count++;
#ifdef CONFIG_FOR_FREEWPC
		if( count > 32)
#else
		if( count > 1)
#endif
		{
			TRACE.loops++;
		}
		else
		{
			if( TRACE.loops )
			{
				dst += sprintf( dst,
					"\n   (loops for %d instructions)\n\n",
					TRACE.loops );
				TRACE.loops = 0;
			}
			if( TRACE.regs[0] )
			{
#ifdef CONFIG_FOR_FREEWPC
				dst += sprintf (dst, "#%u ", activecpu_gettotalcycles ());
#endif
				for( i = 0; i < MAX_REGS && TRACE.regs[i]; i++ )
					dst += sprintf( dst, "%s ", activecpu_dump_reg(TRACE.regs[i]) );
			}
			dst += sprintf( dst, "%0*X: ", addr_width, pc );
			activecpu_dasm( dst, pc );
			strcat( dst, "\n" );
			fprintf( TRACE.file, "%s", buffer );
			fflush(TRACE.file);
			memmove(
				&TRACE.last_pc[0],
				&TRACE.last_pc[1],
				(MAX_LOOPS-1)*sizeof(TRACE.last_pc[0]) );
			TRACE.last_pc[MAX_LOOPS-1] = pc;
		}
	}
}

/**************************************************************************
 * hit_brk_exec
 * Return non zero if execution breakpoint for the active_cpu,
 * the temporary breakpoint or the break on 'active_cpu' was hit
 **************************************************************************/
static int hit_brk_exec(void)
{
	static char dbg_info[63+1];
	UINT32 pc = activecpu_get_pc();

	if( DBG.brk_temp != INVALID && DBG.brk_temp == pc )
	{
		sprintf( dbg_info, "Hit temp breakpoint at $%X", DBG.brk_temp);
		dbg_info_once = dbg_info;
		return 1;
	}
	if( DBG.brk_exec != INVALID && DBG.brk_exec == pc )
	{
		if( DBG.brk_exec_times > 0 )
		{
			if( --DBG.brk_exec_times == 0 )
			{
				sprintf( dbg_info, "Hit exec breakpoint %d times", DBG.brk_exec_reset);
				dbg_info_once = dbg_info;
				return 1;
			}
			return 0;
		}
		sprintf( dbg_info, "Hit exec breakpoint at $%X", DBG.brk_exec);
		dbg_info_once = dbg_info;
		return 1;
	}

	return 0;
}

/**************************************************************************
 * hit_brk_data
 * Return non zero if the data watchpoint for the active_cpu
 * was hit (ie. monitored data changed)
 **************************************************************************/
static int hit_brk_data(void)
{
	static char dbg_info[63+1];
	UINT32 data;

	if( DBG.brk_data == INVALID ) return 0;

//Make WP work if viewing internal data
	if( DBGMEM->internal )
		data = RDINT( DBG.brk_data );
	else
		data = RDMEM(DBG.brk_data);

	if( DBG.brk_data_oldval != data )
	{
		DBG.brk_data_oldval = data;
		if( DBG.brk_data_newval != INVALID )
		{
			if( DBG.brk_data_newval == data )
			{
				sprintf( dbg_info, "Hit data watchpoint at $%X value $%X", DBG.brk_data, DBG.brk_data_newval);
				dbg_info_once = dbg_info;
				return 1;
			}
			return 0;
		}
		sprintf( dbg_info, "Hit data watchpoint at $%X", DBG.brk_data);
		dbg_info_once = dbg_info;
		return 1;
	}
	return 0;
}
#if defined(PINMAME) && defined(DBG_BPR)
/**************************************************************************
 * hit_brk_bpr
 * Return non zero if the bpr watchpoint for the activecpu was hit
 **************************************************************************/
void bpr_memref(UINT32 adr, int length) {
	if (mame_debug && !dbg_fast && !dbg_active) {
		active_cpu = cpu_getactivecpu();
		if (!DBG.ignore && DBG.bprstart != INVALID && DBG.bprnext < 10) {
			DBG.bprdata[DBG.bprnext].adr = adr;
			DBG.bprdata[DBG.bprnext].length = length;
			DBG.bprnext += 1;
		}
	}
}

static int bpr_check(void) {
	static char dbg_info[63+1];
	int ii = 0;

	if (DBG.bprstart == INVALID) return 0;
	while (ii < DBG.bprnext) {
		if ((DBG.bprdata[ii].adr+DBG.bprdata[ii].length-1 >= DBG.bprstart) &&
		    (DBG.bprdata[ii].adr                          <= DBG.bprend)) {
			sprintf(dbg_info, "Hit reference watchpoint between $%X $%X", DBG.bprstart, DBG.bprend);
			dbg_info_once = dbg_info;
			DBG.bprnext = 0;
			return 1;
		}
		ii += 1;
	}
	DBG.bprnext = 0;
	return 0;
}

static void bpr_set(void) {
	char *cmd = CMD;
	int length;

	DBG.bprstart = get_register_or_value(&cmd, &length );
	if (length) {
		DBG.bprstart = rshift(DBG.bprstart) & AMASK; /* EHC 11/14/99: Need to shift + mask otherwise we die */
		DBG.bprend = get_register_or_value(&cmd, &length);
		if (!length) DBG.bprend = DBG.bprstart;
	}
	else DBG.bprstart = INVALID;
	dbg_update = 1;
	edit_cmds_reset();
}
#endif

/**************************************************************************
 * hit_brk_regs
 * Return non zero if the register breakpoint for the active CPU
 * was hit (ie. monitored register changed)
 **************************************************************************/
static int hit_brk_regs(void)
{
	static char dbg_info[63+1];
	UINT32 data;

	if( DBG.brk_regs == INVALID ) return 0;

	data = activecpu_get_reg(DBG.brk_regs);

	if( DBG.brk_regs_oldval != data )
	{
		DBG.brk_regs_oldval = data;
		if( DBG.brk_regs_newval != INVALID )
		{
			if( DBG.brk_regs_newval == (data & DBG.brk_regs_mask) )
			{
				if( DBG.brk_regs_mask != 0xffffffff )
					sprintf( dbg_info, "Hit register %s & $%X watchpoint value $%X", get_register_name(DBG.brk_regs), DBG.brk_regs_mask, DBG.brk_regs_newval);
				else
					sprintf( dbg_info, "Hit register %s watchpoint value $%X", get_register_name(DBG.brk_regs), DBG.brk_regs_newval);
				dbg_info_once = dbg_info;
				return 1;
			}
			return 0;
		}
		sprintf( dbg_info, "Hit register %s watchpoint", get_register_name(DBG.brk_regs));
		dbg_info_once = dbg_info;
		return 1;
	}
	return 0;
}


/**************************************************************************
 * name_rdmem
 * Find a descriptive name for the given memory read region of active_cpu
 **************************************************************************/
static const char *name_rdmem( unsigned base )
{
	static char buffer[16][79+1];
	static int which = 0;
	const struct MachineCPU *cpu = &Machine->drv->cpu[active_cpu];
	const struct Memory_ReadAddress *mr = cpu->memory_read;
	int ram_cnt = 1, nop_cnt = 1;
	const char *name;
	char *dst;

	which = (which+1) % 16;
	dst = buffer[which];
	*dst = '\0';

	while( *dst == '\0' && !IS_MEMPORT_END(mr))
	{
		if (!IS_MEMPORT_MARKER(mr))
		{
			if( base >= mr->start && base <= mr->end )
			{
				unsigned offset = base - mr->start;

				if( (FPTR)mr->handler == (FPTR)MRA_RAM )
					sprintf(dst, "RAM%d+%04X", ram_cnt, lshift(offset) );
				else
				if( (FPTR)mr->handler == (FPTR)MRA_ROM )
				{
					name = name_rom("ROM", REGION_CPU1+active_cpu, &base, mr->start );
					sprintf(dst, "%s+%04X", name, lshift(base) );
				}
				else
				if( (FPTR)mr->handler == (FPTR)MRA_BANK1
				||  (FPTR)mr->handler == (FPTR)MRA_BANK2
				||  (FPTR)mr->handler == (FPTR)MRA_BANK3
				||  (FPTR)mr->handler == (FPTR)MRA_BANK4
				||  (FPTR)mr->handler == (FPTR)MRA_BANK5
				||  (FPTR)mr->handler == (FPTR)MRA_BANK6
				||  (FPTR)mr->handler == (FPTR)MRA_BANK7
				||  (FPTR)mr->handler == (FPTR)MRA_BANK8
				||  (FPTR)mr->handler == (FPTR)MRA_BANK9
				||  (FPTR)mr->handler == (FPTR)MRA_BANK10
				||  (FPTR)mr->handler == (FPTR)MRA_BANK11
				||  (FPTR)mr->handler == (FPTR)MRA_BANK12
				||  (FPTR)mr->handler == (FPTR)MRA_BANK13
				||  (FPTR)mr->handler == (FPTR)MRA_BANK14
				||  (FPTR)mr->handler == (FPTR)MRA_BANK15
				||  (FPTR)mr->handler == (FPTR)MRA_BANK16 )
					sprintf(dst, "BANK%d+%04X", 1 + (int)(MRA_BANK1) - (int)(mr->handler), lshift(offset) );
				else
				if( (FPTR)mr->handler == (FPTR)MRA_NOP )
					sprintf(dst, "NOP%d+%04X", nop_cnt, lshift(offset) );
				else
					if( (FPTR)mr->handler == (FPTR)input_port_0_r )
						sprintf(dst, "input_port_0+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_1_r )
						sprintf(dst, "input_port_1+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_2_r )
						sprintf(dst, "input_port_2+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_3_r )
						sprintf(dst, "input_port_3+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_4_r )
						sprintf(dst, "input_port_4+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_5_r )
						sprintf(dst, "input_port_5+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_6_r )
						sprintf(dst, "input_port_6+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_7_r )
						sprintf(dst, "input_port_7+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_8_r )
						sprintf(dst, "input_port_8+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_9_r )
						sprintf(dst, "input_port_9+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_10_r )
						sprintf(dst, "input_port_10+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_11_r )
						sprintf(dst, "input_port_11+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_12_r )
						sprintf(dst, "input_port_12+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_13_r )
						sprintf(dst, "input_port_13+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_14_r )
						sprintf(dst, "input_port_14+%04X", lshift(offset) );
					else
					if( (FPTR)mr->handler == (FPTR)input_port_15_r )
						sprintf(dst, "input_port_15+%04X", lshift(offset) );
			}
			if( (FPTR)mr->handler == (FPTR)MRA_RAM )
				ram_cnt++;
			else
			if( (FPTR)mr->handler == (FPTR)MRA_NOP )
				nop_cnt++;
		}
		mr++;
	}

	return dst;
}

/**************************************************************************
 * name_wrmem
 * Find a descriptive name for the given memory write region of active_cpu
 **************************************************************************/
static const char *name_wrmem( unsigned base )
{
	static char buffer[16][79+1];
	static int which = 0;
	const struct MachineCPU *cpu = &Machine->drv->cpu[active_cpu];
	const struct Memory_WriteAddress *mw = cpu->memory_write;
	int ram_cnt = 1, nop_cnt = 1;
	const char *name;
	char *dst;

	which = (which+1) % 16;
	dst = buffer[which];
	*dst = '\0';

	ram_cnt = nop_cnt = 1;
	while( *dst == '\0' && !IS_MEMPORT_END(mw))
	{
		if (!IS_MEMPORT_MARKER(mw))
		{
			if( base >= mw->start && base <= mw->end )
			{
				if( (FPTR)mw->handler == (FPTR)MWA_RAM )
					sprintf(dst, "RAM%d+%04X", ram_cnt, lshift(base - mw->start) );
				else
				if( (FPTR)mw->handler == (FPTR)MWA_ROM )
				{
					name = name_rom("ROM", REGION_CPU1+active_cpu, &base, mw->start );
					sprintf(dst, "%s+%04X", name, lshift(base) );
				}
				else
				if( (FPTR)mw->handler == (FPTR)MWA_RAMROM )
				{
					name = name_rom("RAMROM", REGION_CPU1+active_cpu, &base, mw->start);
					sprintf(dst, "%s+%04X", name, lshift(base) );
				}
				else
				if( (FPTR)mw->handler == (FPTR)MWA_BANK1
				||  (FPTR)mw->handler == (FPTR)MWA_BANK2
				||  (FPTR)mw->handler == (FPTR)MWA_BANK3
				||  (FPTR)mw->handler == (FPTR)MWA_BANK4
				||  (FPTR)mw->handler == (FPTR)MWA_BANK5
				||  (FPTR)mw->handler == (FPTR)MWA_BANK6
				||  (FPTR)mw->handler == (FPTR)MWA_BANK7
				||  (FPTR)mw->handler == (FPTR)MWA_BANK8
				||  (FPTR)mw->handler == (FPTR)MWA_BANK9
				||  (FPTR)mw->handler == (FPTR)MWA_BANK10
				||  (FPTR)mw->handler == (FPTR)MWA_BANK11
				||  (FPTR)mw->handler == (FPTR)MWA_BANK12
				||  (FPTR)mw->handler == (FPTR)MWA_BANK13
				||  (FPTR)mw->handler == (FPTR)MWA_BANK14
				||  (FPTR)mw->handler == (FPTR)MWA_BANK15
				||  (FPTR)mw->handler == (FPTR)MWA_BANK16 )
					sprintf(dst, "BANK%d+%04X", 1 + (int)(MWA_BANK1) - (int)(mw->handler), lshift(base - mw->start) );
				else
				if( (FPTR)mw->handler == (FPTR)MWA_NOP )
					sprintf(dst, "NOP%d+%04X", nop_cnt, lshift(base - mw->start) );
			}
			if( (FPTR)mw->handler == (FPTR)MRA_RAM )
				ram_cnt++;
			else
			if( (FPTR)mw->handler == (FPTR)MRA_NOP )
				nop_cnt++;
		}
		mw++;
	}

	return dst;
}

/**************************************************************************
 * name_memory
 * Find a descriptive name for the given memory region of active_cpu
 **************************************************************************/
static const char *name_memory( unsigned base )
{
	static char buffer[8][79+1];
	static int which = 0;
	const char *rd, *wr;

	/* search readmem and writemem names */
	rd = name_rdmem( base );
	wr = name_wrmem( base );

	/* both empty, so it's no specific region */
	if( *rd == '\0' && *wr == '\0' )
	{
		which = (which+1) % 8;
		sprintf(buffer[which], "N/A:%04X", base);
		return buffer[which];
	}

	which = (which+1) % 8;

	/* both names differ? */
	if( strcmp(rd,wr) )
		/* well, return both (separated by tab means left/right aligned) */
		sprintf(buffer[which], "%s\t%s", rd, wr);
	else
		/* return the name for readmem... */
		sprintf(buffer[which], "%s", rd);

	return buffer[which];
}


/**************************************************************************
 * dbg_set_rect
 * set a rectangle from x,y,w and h
 **************************************************************************/
INLINE void dbg_set_rect( struct rectangle *r, int x, int y, int w, int h )
{
	r->min_x = x;
	r->max_x = x + w - 1;
	r->min_y = y;
	r->max_y = y + h - 1;
}


/**************************************************************************
 * dasm_line
 * disassemble <times> instructions from pc and return the final pc
 **************************************************************************/
static unsigned dasm_line( unsigned pc, int times )
{
	static char buffer[127+1];

	while( times-- > 0 )
		pc += activecpu_dasm( buffer, pc );
	pc = lshift( rshift(pc) & AMASK );

	return pc;
}


/**************************************************************************
 * dump_regs
 * Update the register display
 * Compare register values against the ones stored in reg->backup[]
 * Store new values in reg->newval[] which is copied to reg->backup[]
 * before the next instruction is executed (at the end of MAME_Debug).
 **************************************************************************/
static void dump_regs( void )
{
	char title[80+1];
	UINT32 win = WIN_REGS(active_cpu);
	s_regs *regs = &DBGREGS;
	s_edit *pedit = regs->edit;
	UINT32 *old = regs->backup;
	UINT32 *val = regs->newval;
	UINT32 width;
	const char *name = activecpu_name(), *flags = activecpu_flags();
	int w = win_get_w(win);
	int h = win_get_h(win);
	int i, j, l, x, y;
	UINT8 color;
	const INT8 *reg = (INT8*)activecpu_reg_layout();

	/* Called the very first time: find max_width */
	if( regs->count == 0 )
	{
		for(i = 0; reg[i]; i++)
		{
			if( reg[i] == -1 )
				continue;		/* skip row breaks */
			width = strlen( activecpu_dump_reg(reg[i]) );
			if( width >= regs->max_width )
				regs->max_width = width + 1;
		}
	}

	x = 0;
	y = 0;
	win_set_curpos( win, 0, 0 );
	sprintf( title, "CPU #%d %-8s Flags:%s  Cycles:%6u", active_cpu, name, flags, activecpu_get_icount() );
	l = strlen(title);
	if( l + 2 < w )
	{
		/* Everything should fit into the caption */
		if( l + 4 < w )
			/* We can even separate the cycles to the right corner */
			sprintf( title, "CPU #%d %-8s Flags:%s\tCycles:%6u", active_cpu, name, flags, activecpu_get_icount() );
		win_set_title( win, title );
	}
	else
	{
		/* At least CPU # and flags should fit into the caption */
		sprintf( title, "CPU #%d %-8s Flags:%s", active_cpu, name, flags );
		l = strlen(title);
		if( l + 2 < w )
		{
			if( l + 4 < w )
				sprintf( title, "CPU #%d %-8s\tFlags:%s", active_cpu, name, flags );
			win_set_title( win, title );
			if( y < h )
			{
				win_printf( win, "Cycles:%6u\n", activecpu_get_icount() );
			}
			y++;
		}
		else
		{
			sprintf( title, "CPU #%d %-8s Cyc:%6u", active_cpu, name, activecpu_get_icount() );
			l = strlen(title);
			if( l + 2 < w )
			{
				if( l + 4 < w )
					sprintf( title, "CPU #%d %-8s\tCyc:%6u", active_cpu, name, activecpu_get_icount() );
				win_set_title( win, title );
				if( y < h )
				{
					if( strlen(activecpu_flags()) + 8 < w )
						win_printf( win, "Flags: %s\n", flags );
					else
					if( strlen(activecpu_flags()) + 2 < w )
						win_printf( win, "F:%s\n", flags );
					else
						win_printf( win, "%s\n", flags );
				}
				y++;
			}
			else
			{
				/* Only CPU # and name fit into the caption */
				sprintf( title, "CPU #%d %-8s", active_cpu, name );
				l = strlen(title);
				win_set_title( win, title );
				if( y < h )
				{
				if( strlen(activecpu_flags()) + 8 < w )
					win_printf( win, "Flags: %s\n", flags );
				else
				if( strlen(activecpu_flags()) + 2 < w )
					win_printf( win, "F:%s\n", flags );
				else
					win_printf( win, "%s\n", flags );
				}
				y++;
				if( y < h )
				{
					win_printf( win, "Cycles:%6u\n", activecpu_get_icount() );
				}
				y++;
			}
		}
	}
	/* show the scanline position, if appropriate */
	if (dbg_show_scanline)
	{
		win_printf( win, "Scanline: %d Horz: %d\n", cpu_getscanline(), cpu_gethorzbeampos());
		y++;
	}

	regs->top = y;
	y = 0;

	for( i = 0, j = 0; *reg; i++, reg++ )
	{
		if( *reg == -1 )
		{
			if( y >= regs->base && y < regs->base + h - regs->top )
			{
				win_erase_eol( win, ' ' );
				win_putc( win, '\n');
			}
			x = 0;
			y++;
		}
		else
		{
			name = activecpu_dump_reg(*reg);
			if( *name == '\0' )
				continue;

			regs->id[j] = *reg;
			*val = activecpu_get_reg(regs->id[j]);
			color = cur_col[E_REGS];
			if( DBG.brk_regs == *reg )
				color = cur_col[E_BRK_REGS];
			if( *val != *old )
			{
				regs->changed = 1;
				color = (color & 0xf0) | cur_col[E_CHANGES];
			}
			win_set_color( win, color );

			/* edit structure not yet initialized? */
			if( regs->count == 0 )
			{
				const char *p;
				/* Get the cursor position */
				pedit->x = x;
				pedit->y = y + regs->base;
				if( strlen(name) >= regs->max_width )
					regs->max_width = strlen(name) + 1;
				/* Find a colon */
				p = strchr( name, ':' );
				if( p )
				{
					pedit->w = strlen( p + 1 );
				}
				else
				{
					/* Or else find an apostrophe */
					p = strchr( name, '\'' );
					if( p )
					{
						/* Include the apostrophe in the name! */
						++p;
						pedit->w = strlen( p );
					}
					else
					{
						/* TODO: other characters to delimit a register name from it's value? */
						/* this is certainly wrong :( */
						p = name;
						pedit->w = strlen( p );
					}
				}
				/* length of the name (total length - length of nibbles) */
				pedit->n = strlen( name ) - pedit->w;

				/* strip trailing spaces */
				l = p - name;
				while( l != 0 && name[ l - 1 ] == ' ' )
				{
					l--;
				}
				if( l > sizeof( regs->name[ j ] ) - 1 )
				{
					l = sizeof( regs->name[ j ] ) - 1;
				}
				memcpy( regs->name[ j ], name, l );
				regs->name[ j ][ l ] = 0;
			}
			if( y >= regs->base && y < regs->base + h - regs->top )
			{
				win_printf( win, "%s", name );

				win_set_color( win, cur_col[E_REGS] );
				/* If no row break follows, advance to the next tab stop */
				if( reg[1] != -1 )
					win_printf( win, "%*s", regs->max_width - pedit->w - pedit->n, "" );
			}
			x += strlen( name ) + regs->max_width - pedit->w - pedit->n;
			pedit++;
			val++;
			old++;
			j++;
		}
	}
	while( y >= regs->base && y < regs->base + h - regs->top )
	{
		win_erase_eol( win, ' ' );
		win_putc( win, '\n' );
		y++;
	}

	/* Set the total count of registers */
	regs->count = j;
}

/**************************************************************************
 * dump_dasm
 * Update the disassembly display
 **************************************************************************/
static unsigned dump_dasm( unsigned pc )
{
	UINT32 win = WIN_DASM(active_cpu);
	int w = win_get_w(win);
	int h = win_get_h(win);
	int x, y, l, line_pc_cpu = INVALID, line_pc_cur = INVALID;
	UINT8 color;
	char dasm[127+1];
	unsigned pc_first = pc, pc_next;
	unsigned width = (ABITS + 3) / 4;

	while( line_pc_cpu == INVALID )
	{
		pc = pc_first;

		for( y = 0; y < h; y++ )
		{
			win_set_curpos( win, 0, y );
			if( pc == DBG.brk_exec )
				color = cur_col[E_BRK_EXEC];
			else
				color = cur_col[E_DASM];
			if( pc == DBGDASM.pc_cpu )
			{
				color = (color & 0x0f) | (cur_col[E_PC] & 0xf0);
				line_pc_cpu = y;
			}
			if( pc == DBGDASM.pc_cur )
			{
				color = (color & 0x0f) | (cur_col[E_CURSOR] & 0xf0);
				line_pc_cur = y;
			}
			win_set_color( win, color );
			l = win_printf( win, "%0*X: ", width, pc );

			DBGDASM.dst_ea_value = INVALID;
			DBGDASM.src_ea_value = INVALID;
			pc_next = pc + activecpu_dasm( dasm, pc );

			if( DBGDASM.pc_cur == pc )
				win_set_title( win, "%s", get_ea_info(pc) );

			if( dbg_dasm_opcodes )
			{
				unsigned p = rshift(pc);
				unsigned n = rshift(pc_next);
				switch( ALIGN )
				{
				case 1:
					for( x = 0; x < INSTL; x++ )
					{
						if ( p < n )
						{
							l += win_printf( win, "%02X ",
								RDMEM(order(p,1)) );
							p++;
						}
						else l += win_printf( win, "   " );
					}
					break;
				case 2:
					for( x = 0; x < INSTL; x += 2 )
					{
						if ( p < n )
						{
							l += win_printf( win, "%02X%02X ",
								RDMEM(order(p+0,2)), RDMEM(order(p+1,2)) );
							p += 2;
						}
						else l += win_printf( win, "     " );
					}
					break;
				case 4:
					for( x = 0; x < INSTL; x += 4 )
					{
						if ( p < n)
						{
							l += win_printf( win, "%02X%02X%02X%02X ",
								RDMEM(order(p+0,4)), RDMEM(order(p+1,4)),
								RDMEM(order(p+2,4)), RDMEM(order(p+3,4)) );
							p += 4;
						}
						else l += win_printf( win, "         " );
					}
					break;
				}
			}
			pc = lshift(rshift(pc_next) & AMASK);
			switch( dbg_dasm_case )
			{
			case 0: win_printf( win, "%-*.*s", w-l, w-l, dasm ); break;
			case 1: win_printf( win, "%-*.*s", w-l, w-l, lower(dasm) ); break;
			case 2: win_printf( win, "%-*.*s", w-l, w-l, upper(dasm) ); break;
			}
		}
		if( line_pc_cpu == INVALID )
		{
			/*
			 * We didn't find the exact instruction of the CPU PC.
			 * This has to be caused by a jump into the midst of
			 * another instruction down from the top. If the CPU PC
			 * is between pc_first and pc (end), try again on next
			 * instruction size boundary, else bail out...
			 */
			if( DBGDASM.pc_cpu > pc_first && DBGDASM.pc_cpu < pc )
				pc_first += ALIGN;
			else
				line_pc_cpu = 0;
		}
	}

	win_set_curpos( win, 0, line_pc_cur );

	return pc;
}

/**************************************************************************
 * dump_mem_hex
 * Update a memory window using the cpu_readmemXXX function
 * Changed values are displayed using foreground color cur_col[E_CHANGES]
 * The new values are stored into mem->newval[] of the active_cpu
 **************************************************************************/
static void dump_mem_hex( int which, unsigned len_addr, unsigned len_data )
{
	UINT32 win = WIN_MEM(active_cpu,which);
	s_edit *pedit = DBGMEM[which].edit;
	int w = win_get_w(win);
	int h = win_get_h(win);
	UINT8 *old = DBGMEM[which].backup;
	UINT8 *val = DBGMEM[which].newval;
	UINT8 color, dim_bright = 0;
	UINT8 spc_left = 0; 	/* assume no space left of address */
	UINT8 spc_addr = 0; 	/* assume no space after address */
	UINT8 spc_data = 1; 	/* assume one space between adjacent data elements */
	UINT8 spc_hyphen = 0;	/* assume no space around center hyphen */
	unsigned offs, column;

	/* how many elements (bytes,words,dwords) will fit in a line? */
	DBGMEM[which].width = (w - len_addr - 1) / (len_data + spc_data);

	/* display multiples of eight bytes per line only */
	if( DBGMEM[which].width > ((16/len_data)-1) )
		DBGMEM[which].width &= ~((16/len_data)-1);

	/* Is bytes per line not divideable by eight? */
	if( dbg_mem_squeezed && (DBGMEM[which].width & 7) )
	{
		/* We try an alternating dim,bright layout w/o data spacing */
		spc_data = 0;
		/* how many bytes will fit in a line? */
		DBGMEM[which].width = (w - len_addr - 1) / len_data;
		/* display multiples of eight data elements per line only */
		if( DBGMEM[which].width > ((16/len_data)-1) )
			DBGMEM[which].width &= ~((16/len_data)-1);
		dim_bright = 0x08;
	}

	/* calculate number of bytes per line */
	DBGMEM[which].bytes = DBGMEM[which].width * len_data / 2;
	/* calculate the DBGMEM[which].size using that data width */
	DBGMEM[which].size = DBGMEM[which].bytes * h;

	/* will a space after the address fit into the line? */
	if( ( len_addr + spc_addr + DBGMEM[which].width * (len_data + spc_data) + 1 ) < w )
		spc_addr = 1;

	/* will two spaces around the center hyphen fit into the line ? */
	if( ( len_addr + spc_addr + DBGMEM[which].width * (len_data + spc_data) + 2 ) < w )
		spc_hyphen = 1;

	while( ( 2*spc_left + len_addr + spc_addr + DBGMEM[which].width * (len_data + spc_data) - 1 + spc_hyphen ) + 2 < w )
		spc_left++;

	win_set_curpos( win, 0, 0 );

	for( offs = 0, column = 0; offs < DBGMEM[which].size; offs++, old++, val++ )
	{
		color = cur_col[E_MEM1+which];
		switch( len_data )
		{
		case 2: /* UINT8 mode */
			DBGMEM[which].address = (DBGMEM[which].base + order(offs,1)) & AMASK;
			break;
		case 4: /* UINT16 mode */
			DBGMEM[which].address = (DBGMEM[which].base + order(offs,2)) & AMASK;
			break;
		case 8: /* UINT32 mode */
			DBGMEM[which].address = (DBGMEM[which].base + order(offs,4)) & AMASK;
			break;
		}

		if( column == 0 )
		{
			win_set_color( win, cur_col[E_MEM1+which] );
			win_printf( win, "%*s%0*X:%*s",
				spc_left, "", len_addr, lshift((DBGMEM[which].base + offs) & AMASK), spc_addr, "" );
		}

		if( DBGMEM[which].address == DBG.brk_data )
			color = cur_col[E_BRK_DATA];

		if( DBGMEM[which].internal )
			*val = RDINT( DBGMEM[which].address );
		else
		if( DBGMEM[which].pgm_memory_base )
			*val = OP_ROM[DBGMEM[which].pgm_memory_base + DBGMEM[which].address];
		else
			*val = RDMEM( DBGMEM[which].address );

		if( *val != *old )
		{
			DBGMEM[which].changed = 1;
			color = (color & 0xf0) | (cur_col[E_CHANGES] & 0x0f);
		}

		if( (column * 2 / len_data) & 1 )
			color ^= dim_bright;

		/* store memory edit x,y */
		pedit->x = win_get_cx( win );
		pedit->y = win_get_cy( win );
		pedit->w = 2;
		pedit->n = order(column % (len_data / 2), len_data / 2);
		pedit++;

#ifdef PINMAME
		//COLOR where STACK currently points!
		if(activecpu_get_sp()==DBGMEM[which].address)
			color = cur_col[COLOR_PROMPT];	// was [COLOR_PROMPT*16]; there's only 0-15; ToDo: mistake or "special feature"?
#endif

		win_set_color( win, color );
		switch( DBGMEM[which].ascii )
		{
		case MODE_CHR_HEX:
			win_printf( win, "%02X", *val );
			break;
		case MODE_CHR_XLATE:
			win_printf( win, "%c ", trans_table[*val] );
			break;
		case MODE_CHR_PLAIN:
			if( *val == '\0' || *val == '\b' || *val == '\t' ||
				*val == '\r' || *val == '\n' )
				win_printf( win, ". " );
			else
				win_printf( win, "%c ", *val );
			break;
		}

		if( ++column < DBGMEM[which].bytes )
		{
			win_set_color( win, cur_col[E_MEM1+which] );
			if( column == DBGMEM[which].bytes / 2 )
				win_printf( win, "%*s-%*s", spc_hyphen, "", spc_hyphen, "" );
			else
			if( spc_data && (column * 2 % len_data) == 0 )
				win_putc( win, ' ' );
		}
		else
		{
			win_putc( win, '\n');
			column = 0;
		}
	}
}

/**************************************************************************
 * dump_mem
 * Update a memory window
 * Dispatch to one of the memory handler specific output functions
 **************************************************************************/
static void dump_mem( int which, int set_title )
{
	unsigned len_addr = (ABITS + ASHIFT + 3) / 4;

	if( set_title )
	{
		if( DBGMEM[which].internal )
			win_set_title( WIN_MEM(active_cpu,which), "CPU internal" );
		else
			win_set_title( WIN_MEM(active_cpu,which), name_memory(DBGMEM[which].base + DBGMEM[which].pgm_memory_base) );
	}

	switch( DBGMEM[which].mode )
	{
	case MODE_HEX_UINT8:  dump_mem_hex( which, len_addr, 2 ); break;
	case MODE_HEX_UINT16: dump_mem_hex( which, len_addr, 4 ); break;
	case MODE_HEX_UINT32: dump_mem_hex( which, len_addr, 8 ); break;
	}
}

/**************************************************************************
 * edit_regs
 * Edit the registers
 **************************************************************************/
static void edit_regs( void )
{
	UINT32 win = WIN_REGS(active_cpu);
	s_regs *regs = &DBGREGS;
	s_edit *pedit = regs->edit;
	unsigned shift, mask, val;
	const char *k;
	int i, x, y;

	/* Eventually update the cmdline window caption */
	edit_cmds_info();

	if( regs->base > pedit[ regs->idx ].y )
	{
		regs->base = pedit[ regs->idx ].y;
		dump_regs();
	}
	else
	if( pedit[ regs->idx ].y >= regs->base + win_get_h( win ) - regs->top )
	{
		regs->base = pedit[ regs->idx ].y - win_get_h( win ) + regs->top + 1;
		dump_regs();
	}
	win_set_curpos( win, pedit[regs->idx].x + pedit[regs->idx].n + regs->nibble, pedit[regs->idx].y - regs->base + regs->top);
	set_screen_curpos( win_get_cx_abs(win), win_get_cy_abs(win) );

	i = readkey();
	k = keyboard_name(i);

	shift = ( pedit[ regs->idx ].w - 1 - regs->nibble ) * 4;
	mask = ~(0x0000000f << shift);

	if( strlen(k) == 1 )
	{
		switch( k[0] )
		{
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9': case 'A': case 'B':
		case 'C': case 'D': case 'E': case 'F':
			val = k[0] - '0';
			if( val > 9 ) val -= 7;
			val <<= shift;
			/* now modify the register */
			activecpu_set_reg( regs->id[regs->idx],
				( activecpu_get_reg( regs->id[regs->idx] ) & mask ) | val );
			dump_regs();
			i = KEYCODE_RIGHT;	/* advance to next nibble */
		}
	}

	switch( i )
	{
	case KEYCODE_LEFT:
		if( --regs->nibble < 0 )
		{
			if( --regs->idx < 0 )
			{
				regs->idx = regs->count - 1;
			}
			regs->nibble = pedit[regs->idx].w - 1;
		}
		break;

	case KEYCODE_RIGHT:
		if( ++regs->nibble >= pedit[regs->idx].w )
		{
			regs->nibble = 0;
			if( ++regs->idx >= regs->count )
			{
				regs->idx = 0;
			}
		}
		break;

	case KEYCODE_UP:
		i = regs->idx;
		x = pedit[regs->idx].x;
		y = pedit[regs->idx].y;
		while( x != pedit[i].x || pedit[i].y == y )
		{
			if( --i < 0 )
			{
				i = regs->count - 1;
				if( pedit[i].y == y )
				{
					i = regs->idx;
					break;
				}
			}
		}
		if( i != regs->idx )
		{
			if( regs->nibble >= pedit[i].w )
				regs->nibble = pedit[i].w - 1;
			regs->idx = i;
		}
		break;

	case KEYCODE_DOWN:
		i = regs->idx;
		x = pedit[regs->idx].x;
		y = pedit[regs->idx].y;
		while( x != pedit[i].x || pedit[i].y == y )
		{
			if( ++i >= regs->count )
			{
				i = 0;
				if( pedit[i].y == y )
				{
					i = regs->idx;
					break;
				}
			}
		}
		if( i != regs->idx )
		{
			if( regs->nibble >= pedit[i].w )
				regs->nibble = pedit[i].w - 1;
			regs->idx = i;
		}
		break;

	case KEYCODE_ENTER:
		DBG.window = EDIT_CMDS;
		break;

	default:
		cmd_default( i );
	}
}


/**************************************************************************
 * edit_dasm
 * Edit the disassembly output
 **************************************************************************/
static void edit_dasm(void)
{
	UINT32 win = WIN_DASM(active_cpu);
	const char *k;
	int i, update_window = 0;

	/* Eventually update the cmdline window caption */
	edit_cmds_info();

	set_screen_curpos( win_get_cx_abs(win), win_get_cy_abs(win) );

	i = readkey();
	k = keyboard_name(i);

	switch( i )
	{
	case KEYCODE_M: /* Toggle mode (opcode display) */
		sprintf( CMD, "%d", dbg_dasm_opcodes ^ 1 );
		cmd_set_dasm_opcodes();
		break;

	case KEYCODE_D: /* Default case disassembly */
		dbg_dasm_case = 0;
		update_window = 1;
		break;

	case KEYCODE_L: /* Lower case disassembly */
		dbg_dasm_case = 1;
		update_window = 1;
		break;

	case KEYCODE_U: /* Upper case disassembly */
		dbg_dasm_case = 2;
		update_window = 1;
		break;

	case KEYCODE_R: /* Toggle relative jumps display */
		dbg_dasm_relative_jumps ^= 1;
		update_window = 1;
		break;

	case KEYCODE_ENTER:
		DBG.window = EDIT_CMDS;
		break;

	default:
		cmd_default( i );
	}
	if( update_window )
	{
		DBGDASM.pc_end = dump_dasm( DBGDASM.pc_top );
	}
}


/**************************************************************************
 * edit_mem
 * Edit the memory dumps output
 **************************************************************************/
static void edit_mem( int which )
{
	UINT32 win = WIN_MEM(active_cpu,which);
	s_edit *pedit = DBGMEM[which].edit;
	const char *k;
	unsigned shift, mask, val;
	int i, update_window = 0;

	/* Eventually update the cmdline window caption */
	edit_cmds_info();

	win_set_curpos( win, pedit[DBGMEM[which].offset].x + DBGMEM[which].nibble, pedit[DBGMEM[which].offset].y );
	set_screen_curpos( win_get_cx_abs(win), win_get_cy_abs(win) );

	switch( DBGMEM[which].mode )
	{
		case MODE_HEX_UINT8:
			DBGMEM[which].address = (DBGMEM[which].base + DBGMEM[which].offset) & AMASK;
			break;
		case MODE_HEX_UINT16:
			DBGMEM[which].address = (DBGMEM[which].base + (DBGMEM[which].offset & ~1) + pedit[DBGMEM[which].offset].n ) & AMASK;
			break;
		case MODE_HEX_UINT32:
			DBGMEM[which].address = (DBGMEM[which].base + (DBGMEM[which].offset & ~3) + pedit[DBGMEM[which].offset].n ) & AMASK;
			break;
	}
	if ( !DBGMEM[which].internal )
		win_set_title( win, name_memory(DBGMEM[which].address + DBGMEM[which].pgm_memory_base) );

	i = readkey();
	k = keyboard_name(i);

	shift = (pedit[DBGMEM[which].offset].w - 1 - DBGMEM[which].nibble) * 4;
	mask = ~(0x0f << shift);

	if( strlen(k) == 1 )
	{
		switch( k[0] )
		{
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9': case 'A': case 'B':
		case 'C': case 'D': case 'E': case 'F':
			val = k[0] - '0';
			if( val > 9 ) val -= 7;
			val <<= shift;
			/* now modify the register */
			if( DBGMEM[which].internal )
			{
				if( cputype_get_interface(cputype)->internal_write )
					WRINT( DBGMEM[which].address, ( RDINT( DBGMEM[which].address ) & mask ) | val );
			}
			else
			if( DBGMEM[which].pgm_memory_base == 0 )
				WRMEM( DBGMEM[which].address, ( RDMEM( DBGMEM[which].address ) & mask ) | val );
			/* we don't write to 'program memory' */
			update_window = 1;
			i = KEYCODE_RIGHT;	/* advance to next nibble */
		}
	}

	switch( i )
	{
	case KEYCODE_LEFT:
		if( --DBGMEM[which].nibble < 0 )
		{
			if( --DBGMEM[which].offset < 0 )
			{
				DBGMEM[which].base = (DBGMEM[which].base - DBGMEM[which].bytes) & AMASK;
				DBGMEM[which].offset += DBGMEM[which].bytes;
				update_window = 1;
			}
			DBGMEM[which].nibble = pedit[DBGMEM[which].offset].w - 1;
		}
		break;

	case KEYCODE_RIGHT:
		if( ++DBGMEM[which].nibble >= pedit[DBGMEM[which].offset].w )
		{
			DBGMEM[which].nibble = 0;
			if( ++DBGMEM[which].offset >= DBGMEM[which].size )
			{
				DBGMEM[which].base = (DBGMEM[which].base + DBGMEM[which].bytes) & AMASK;
				DBGMEM[which].offset -= DBGMEM[which].bytes;
				update_window = 1;
			}
		}
		break;

	case KEYCODE_UP:
		DBGMEM[which].offset -= DBGMEM[which].bytes;
		if( DBGMEM[which].offset < 0 )
		{
			DBGMEM[which].base = (DBGMEM[which].base - DBGMEM[which].bytes) & AMASK;
			DBGMEM[which].offset += DBGMEM[which].bytes;
			update_window = 1;
		}
		break;

	case KEYCODE_DOWN:
		DBGMEM[which].offset += DBGMEM[which].bytes;
		if( DBGMEM[which].offset >= DBGMEM[which].size )
		{
			DBGMEM[which].base = (DBGMEM[which].base + DBGMEM[which].bytes) & AMASK;
			DBGMEM[which].offset -= DBGMEM[which].bytes;
			update_window = 1;
		}
		break;

	case KEYCODE_PGUP:
		DBGMEM[which].base = (DBGMEM[which].base - DBGMEM[which].size) & AMASK;
		update_window = 1;
		break;

	case KEYCODE_PGDN:
		DBGMEM[which].base = (DBGMEM[which].base + DBGMEM[which].size) & AMASK;
		update_window = 1;
		break;

	case KEYCODE_HOME:
		DBGMEM[which].offset = 0;
		DBGMEM[which].base = 0x00000000;
		update_window = 1;
		break;

	case KEYCODE_END:
		DBGMEM[which].offset = DBGMEM[which].size - 1;
		DBGMEM[which].base = (0xffffffff - DBGMEM[which].offset) & AMASK;
		update_window = 1;
		break;

	case KEYCODE_H:
		DBGMEM[which].ascii = (DBGMEM[which].ascii + 1) % MODE_CHR_COUNT;
		update_window = 1;
		break;

	case KEYCODE_M: /* display mode */
		DBGMEM[which].mode = (DBGMEM[which].mode + 1) % MODE_HEX_COUNT;
		/* Reset cursor coordinates and sizes of the edit info */
		memset( DBGMEM[which].edit, 0, sizeof(DBGMEM[which].edit) );
		update_window = 1;
		break;

	case KEYCODE_P: /* program memory */
		DBGMEM[which].pgm_memory_base ^= PGM_MEMORY;
		/* Reset cursor coordinates and sizes of the edit info */
		memset( DBGMEM[which].edit, 0, sizeof(DBGMEM[which].edit) );
		update_window = 1;
		break;

	case KEYCODE_I: /* internal memory */
		if( cputype_get_interface(cputype)->internal_read )
		{
			DBGMEM[which].internal ^= 1;
			/* Reset cursor coordinates and sizes of the edit info */
			memset( DBGMEM[which].edit, 0, sizeof(DBGMEM[which].edit) );
			update_window = 1;
		}
		break;

	case KEYCODE_ENTER:
		DBG.window = EDIT_CMDS;
		break;

	default:
		cmd_default( i );
		update_window = 1;
	}

	if( update_window )
	{
		memcpy( DBGMEM[which].backup, DBGMEM[which].newval, DBGMEM[which].size );
		dump_mem( which, DBGMEM[which].internal );
	}
}

/**************************************************************************
 * edit_cmds_info
 * Search the cmdline for the beginning of a known command and
 * display some information in the caption of the command line input
 **************************************************************************/
static int edit_cmds_info( void )
{
	char *cmd = CMD;
	char hist_info[31+1];
	int i, l;

	hist_info[0] = '\0';
	if( DBG.hist_cnt )
		sprintf( hist_info, "\tHistory: %d", DBG.hist_cnt);

	if( strlen(cmd) )
	{
		for( i = 0; commands[i].name; i++ )
		{
			l = strlen(cmd);
			if( strlen(commands[i].name) < l )
				l = strlen(commands[i].name);
			if( strncmp( cmd, commands[i].name, l ) == 0 && !isalnum(cmd[l]) )
			{
				win_set_title( WIN_CMDS(active_cpu), "Command: %s %s%s",
					commands[i].name, commands[i].args, hist_info );
				return i;
			}
			if( commands[i].alias )
			{
				l = strlen(cmd);
				if( strlen(commands[i].alias) < l )
					l = strlen(commands[i].alias);
				if( strncmp( cmd, commands[i].alias, l ) == 0 && !isalnum(cmd[l]) )
				{
					win_set_title( WIN_CMDS(active_cpu), "Command: %s %s (aka %s)%s",
						commands[i].alias, commands[i].args, commands[i].name, hist_info );
					return i;
				}
			}
		}
	}
	if( dbg_info_once )
	{
		win_set_title( WIN_CMDS(active_cpu), "%s%s", dbg_info_once, hist_info );
		dbg_info_once = NULL;
	}
	else
	{
		win_set_title( WIN_CMDS(active_cpu), "Command (press F1 for help)%s", hist_info );
	}
	return INVALID;
}

/**************************************************************************
 * edit_cmds_parse
 * Search the cmdline for a known command and if found,
 * strip it from cmdline and return it's index
 **************************************************************************/
static int edit_cmds_parse( char *cmdline )
{
	int i, l;

	for( i = 0; commands[i].valid; i++ )
	{
		if( !commands[i].name )
			continue;
		l = strlen( commands[i].name );
		if( !strncmp( cmdline, commands[i].name, l ) && !isalnum( cmdline[l] ) )
		{
			while( cmdline[l] && isspace( cmdline[l] ) ) l++;
			strcpy( cmdline, cmdline + l );
			return i;
		}
		if( commands[i].alias )
		{
			l = strlen( commands[i].alias );
			if( !strncmp( cmdline, commands[i].alias, l ) && !isalnum( cmdline[l] ) )
			{
				while( cmdline[l] && isspace( cmdline[l] ) ) l++;
				strcpy( cmdline, cmdline + l );
				return i;
			}
		}
	}
	return INVALID;
}


/**************************************************************************
 * edit_cmds_append
 * Append a character (string) to the command line
 **************************************************************************/
static void edit_cmds_append( const char *src )
{
	char *cmdline = DBG.cmdline;
	UINT32 win = WIN_CMDS(active_cpu);
	if( strlen(cmdline) < 80 )
	{
		strcat(cmdline, src);
		win_printf( win, "%s", src );
	}
}

void edit_cmds_reset( void )
{
	unsigned win = WIN_CMDS(active_cpu);
	DBG.cmdline[0] = '\0';
	win_set_color( win, cur_col[E_CMDS] );
	win_set_curpos( win, 0, 0 );
	set_screen_curpos( win_get_cx_abs(win), win_get_cy_abs(win) );
	win_erase_eol( win, ' ' );
}

/**************************************************************************
 * edit_cmds
 * Edit the command line input
 **************************************************************************/
static void edit_cmds(void)
{
	char *cmdline = DBG.cmdline;
	UINT32 win = WIN_CMDS(active_cpu);
	const char *k;
	int i, l, cmd;

	set_screen_curpos( win_get_cx_abs(win), win_get_cy_abs(win) );

	cmd = edit_cmds_info();

	i = readkey();
	k = keyboard_name(i);
	l = strlen(k);

	if( l == 1 )
		edit_cmds_append(k);

	switch( i )
	{
	case KEYCODE_SPACE:
		/*
		 * Command completion for convenience:
		 * found a valid command and no space in the command line yet?
		 */
		if( cmd != INVALID && strchr(CMD, ' ') == NULL )
		{
			strcpy( CMD, commands[cmd].name );
			win_set_curpos( win, 0, 0 );
			win_printf( win, "%s", CMD );
		}
		edit_cmds_append(" ");
		break;

	case KEYCODE_BACKSPACE:
		if( strlen(cmdline) > 0 )
		{
			cmdline[strlen(cmdline)-1] = '\0';
			win_printf( win, "\b \b" );
		}
		break;

	case KEYCODE_ENTER:
		if( strlen(cmdline) )
		{
			cmd = edit_cmds_parse( cmdline );
			if( cmd != INVALID && commands[cmd].function )
				(*commands[cmd].function)();
			break;
		}
		else
		{
#ifdef PINMAME
			/*Did user specify a SHIFT-ENTER keyboard combination?*/
			if(shift_pressed) {
				cmd_jumpover();
			}
			else
#endif
			/* ENTER in an empty line: do single step... */
			i = KEYCODE_F8;
		}
		/* fall through */
	default:
		cmd_default( i );
	}
}

/**************************************************************************
 **************************************************************************
 *
 *		Command functions
 *
 **************************************************************************
 **************************************************************************/

/**************************************************************************
 * cmd_default
 * Handle a key stroke with no special meaning inside the active window
 **************************************************************************/
static void cmd_default( int code )
{
	int i;
	for( i = 0; commands[i].valid; i++ )
	{
		if( (commands[i].valid & (1<<DBG.window)) && commands[i].key == code )
		{
			if( commands[i].function )
				(*commands[i].function)();
			return;
		}
	}
}


/**************************************************************************
 * cmd_set_element_color
 * Set a specific color foreground and background
 **************************************************************************/
static void cmd_set_element_color( void )
{
	char *cmd = CMD;
	unsigned element, fg, bg, win;
	int length;

	element = get_option_or_value( &cmd, &length, ELEMENT_NAMES );
	if( length )
	{
		fg = get_option_or_value( &cmd, &length, COLOR_NAMES );
		if( length )
		{
			bg = get_option_or_value( &cmd, &length, COLOR_NAMES );
			if( !length ) bg = 0;	/* BLACK is default background */
		}
		else
		{
			bg = def_col[element] >> 4;
			fg = def_col[element] & 15;
		}
		cur_col[element] = fg + 16 * bg;
		switch( element )
		{
			case E_TITLE:
				for( win = 0; win < MAX_WINDOWS; win++ )
					win_set_title_color( win, cur_col[element] );
				break;
			case E_FRAME:
				for( win = 0; win < MAX_WINDOWS; win++ )
					win_set_frame_color( win, cur_col[element] );
				break;
		}
	}
	else
	{
		memcpy( cur_col, def_col, sizeof(cur_col) );
		for( win = 0; win < MAX_WINDOWS; win++ )
			win_set_title_color( win, cur_col[E_TITLE] );
		for( win = 0; win < MAX_WINDOWS; win++ )
			win_set_frame_color( win, cur_col[E_FRAME] );
	}

	edit_cmds_reset();
	dbg_update = 1;
}

/**************************************************************************
 * cmd_brk_regs_set
 * Set the register breakpoint for the current CPU
 **************************************************************************/
static void cmd_brk_regs_set( void )
{
	char *cmd = CMD;
	unsigned data;
	int length;

	DBG.brk_regs = get_register_id( &cmd, &length );
	if( DBG.brk_regs > 0 )
	{
		DBG.brk_regs_oldval = activecpu_get_reg(DBG.brk_regs);
		data = get_register_or_value( &cmd, &length );
		if( length )
		{
			DBG.brk_regs_newval = data;
			data = get_register_or_value( &cmd, &length );
			if( length )
			{
				DBG.brk_regs_mask = data;
				/* Remove masked bits from the new value too ;-) */
				DBG.brk_regs_newval &= data;
			}
			else
			{
				DBG.brk_regs_mask = 0xffffffff;
			}
		}
		else
		{
			DBG.brk_regs_newval = INVALID;
			DBG.brk_regs_mask = 0xffffffff;
		}
	}
	dbg_update = 1;
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_brk_regs_clear
 * Reset the watchpoint for the current CPU to INVALID
 **************************************************************************/
static void cmd_brk_regs_clear( void )
{
	if( DBG.brk_regs != INVALID )
	{
		DBG.brk_regs = INVALID;
		DBG.brk_regs_oldval = INVALID;
		DBG.brk_regs_newval = INVALID;
		DBG.brk_regs_mask = 0xffffffff;
		dbg_update = 1;
	}
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_brk_data_set
 * Set the watchpoint for the current CPU to the specified address
 * The monitored data is one byte at the given address
 **************************************************************************/
static void cmd_brk_data_set( void )
{
	char *cmd = CMD;
	unsigned data;
	int length;

	DBG.brk_data = get_register_or_value( &cmd, &length );

	DBG.brk_data = rshift(DBG.brk_data) & AMASK; /* EHC 11/14/99: Need to shift + mask otherwise we die */

	if( length )
	{
//Make WP work if viewing internal data
#ifdef PINMAME
		if( DBGMEM->internal )
			data = RDINT( DBG.brk_data );
		else
#endif
		data = RDMEM(DBG.brk_data);

		DBG.brk_data_oldval = data;
		data = get_register_or_value( &cmd, &length );

		if( length )
		{
			DBG.brk_data_newval = data;
		}
		else
		{
			DBG.brk_data_newval = INVALID;
		}
	}

	dbg_update = 1;
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_brk_data_clear
 * Reset the watchpoint for the current CPU to INVALID
 **************************************************************************/
static void cmd_brk_data_clear( void )
{
	if( DBG.brk_data != INVALID )
	{
		DBG.brk_data = INVALID;
		DBG.brk_data_oldval = INVALID;
		DBG.brk_data_newval = INVALID;
		dbg_update = 1;
	}
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_brk_exec_set
 * Set the execution breakpoint for the current CPU to the specified address
 **************************************************************************/
static void cmd_brk_exec_set( void )
{
	char *cmd = CMD;
	unsigned times;
	int length;

	DBG.brk_exec = get_register_or_value( &cmd, &length );
	if( length )
	{
		times = get_register_or_value( &cmd, &length );
		if( length )
		{
			DBG.brk_exec_times = times;
			DBG.brk_exec_reset = times;
		}
		else
		{
			DBG.brk_exec_times = 0;
			DBG.brk_exec_reset = 0;
		}
	}
	else
	{
		DBG.brk_exec = DBGDASM.pc_cur;
		DBG.brk_exec_times = 0;
		DBG.brk_exec_reset = 0;
	}
	dbg_update = 1;
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_brk_exec_clear
 * Reset the execution breakpoint for the current CPU to INVALID
 **************************************************************************/
static void cmd_brk_exec_clear( void )
{
	if( DBG.brk_exec != INVALID )
	{
		DBG.brk_exec = INVALID;
		DBG.brk_exec_times = 0;
		DBG.brk_exec_reset = 0;
		dbg_update = 1;
	}
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_display_memory
 * Set one of the memory display window's start address
 **************************************************************************/
static void cmd_display_memory( void )
{
	char *cmd = CMD;
	unsigned which, address;
	int length;

	which = xtou( &cmd, &length );
	if( length )
	{
		address = get_register_or_value( &cmd, &length );
		if( length )
		{
			which = (which - 1) % MAX_MEM;
		}
		else
		{
			address = which;
			which = 0;
		}
		address = rshift(address) & AMASK;
		DBGMEM[which].base = address;
		dump_mem( which, 1 );
	}
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_dasm_to_file
 * Disassemble a range of code and output it to a file
 **************************************************************************/
static void cmd_dasm_to_file( void )
{
	char buffer[127+1], *cmd = CMD;
	const char *filename;
	int length;
	FILE *file;
	unsigned i, pc, size, start, end, width, opcodes;

	filename = get_file_name( &cmd, &length );
	if( !length )
	{
		win_msgbox( cur_col[E_ERROR], "DASM arguments",
			"Filename missing");
		edit_cmds_reset();
		return;
	}
	start = get_register_or_value( &cmd, &length );
	if( !length )
	{
		win_msgbox( cur_col[E_ERROR], "DASM arguments",
			"Start address missing");
		edit_cmds_reset();
		return;
	}
	end = get_register_or_value( &cmd, &length );
	if( !length )
	{
		win_msgbox( cur_col[E_ERROR], "DASM arguments",
			"End address missing");
		edit_cmds_reset();
		return;
	}
	opcodes = get_boolean( &cmd, &length );
	if( length == 4 ) opcodes = 1; 	/* default to display opcodes */

	file = fopen(filename, "w");
	if( !file )
	{
		win_msgbox( cur_col[E_ERROR], "DASM to file",
			"Could not create %s", filename);
		edit_cmds_reset();
		return;
	}

	width = (ABITS + ASHIFT + 3) / 4;

	for( pc = start; pc <= end; /* */ )
	{
		unsigned p = rshift(pc);
		unsigned s;
		size = activecpu_dasm( buffer, pc );
		s = rshift(size);

		fprintf(file, "%0*X: ", width, pc );
		if( opcodes )
		{
		switch( ALIGN )
		{
		case 1: /* dump bytes */
			for( i = 0; i < INSTL; i++ )
			{
				if ( i < s )
					fprintf( file, "%02X ", RDMEM(order(p+i,1)) );
				else
					fprintf( file, "   ");
			}
			break;
		case 2: /* dump words */
			for( i = 0; i < INSTL; i += 2 )
			{
				if ( i < s )
					fprintf( file, "%02X%02X ",
						RDMEM(order(p+i+0,2)), RDMEM(order(p+i+1,2)) );
				else
					fprintf( file, "     ");
			}
			break;
		case 4: /* dump dwords */
			for( i = 0; i < INSTL; i += 4 )
			{
				if ( i < s )
					fprintf( file, "%02X%02X%02X%02X ",
						RDMEM(order(p+i+0,4)), RDMEM(order(p+i+1,4)),
						RDMEM(order(p+i+2,4)), RDMEM(order(p+i+3,4)) );
				else
					fprintf( file, "     ");
			}
			break;
		}
		fprintf( file, " ");	// extra space for better separation of opcodes and disassembly
		}

		fprintf( file, "%s\n", buffer );
		if( (pc + size) < pc )
			break;
		pc += size;
	}
	fclose( file );

	edit_cmds_reset();
}

/**************************************************************************
 * cmd_dump_to_file
 * (Hex-)Dump a range of code and output it to a file
 **************************************************************************/
static void cmd_dump_to_file( void )
{
	UINT8 buffer[16];
	char *cmd = CMD;
	const char *filename;
	int length;
	FILE *file;
	unsigned x, offs, address = 0, start, end, width, data;
	unsigned datasize, asciimode, pgm_memory_base;

	filename = get_file_name( &cmd, &length );
	if( !length )
	{
		win_msgbox( cur_col[E_ERROR], "DUMP arguments",
			"<filename> missing");
		edit_cmds_reset();
		return;
	}
	start = get_register_or_value( &cmd, &length );
	if( !length )
	{
		win_msgbox( cur_col[E_ERROR], "DUMP arguments",
			"<start> address missing");
		edit_cmds_reset();
		return;
	}
	start = rshift(start);
	end = get_register_or_value( &cmd, &length );
	if( !length )
	{
		win_msgbox( cur_col[E_ERROR], "DUMP arguments",
			"<end> address missing");
		edit_cmds_reset();
		return;
	}
	end = rshift(end);
	asciimode = 1;			/* default to translation table */
	pgm_memory_base = 0;	/* default to data mode (offset 0) */
	datasize = ALIGN*2; 	/* default to align unit of that CPU */
	data = get_option_or_value( &cmd, &length, "BYTE\0WORD\0DWORD\0");
	if( length )
	{
		if( data > 2 )
		{
			win_msgbox( cur_col[E_ERROR], "DUMP arguments",
				"Wrong <data size>. Only BYTE, WORD or DWORD\n(also 0, 1 or 2) are supported");
			data = 1;
		}
		else
		{
			datasize = 2 << data;
		}
		/* look if there's also an ASCII mode specified */
		data = get_option_or_value( &cmd, &length, "OFF\0TRANSLATE\0FULL\0" );
		if( length )
		{
			if( data > 2 )
			{
				win_msgbox( cur_col[E_ERROR], "DUMP arguments",
					"Wrong ASCII mode. Only OFF, TRANSLATE or FULL\n(also 0,1 or 2) are supported");
				data = 1;
			}
			asciimode = data;
			/* look if there's also an PROG/DATA mode specified */
			data = get_option_or_value( &cmd, &length, "PROG\0DATA\0" );
			if( length )
			{
				if( data > 1 )
				{
					win_msgbox( cur_col[E_ERROR], "DUMP arguments",
						"Wrong PROG/DATA mode. Only PROG or DATA\n(also 0,1) are supported");
					data = 1;
				}
				pgm_memory_base = data;
			}
		}
	}

	if( pgm_memory_base )
		pgm_memory_base = PGM_MEMORY;

	file = fopen(filename, "w");
	if( !file )
	{
		win_msgbox( cur_col[E_ERROR], "DUMP to file",
			"Could not create %s", filename);
		edit_cmds_reset();
		return;
	}

	width = (ABITS + ASHIFT + 3) / 4;

	for( x = 0, offs = 0; offs + start <= end; offs++ )
	{
		switch( datasize )
		{
			case 2:
				address = (start + order(offs,1)) & AMASK;
				break;
			case 4:
				address = (start + order(offs,2)) & AMASK;
				break;
			case 8:
				address = (start + order(offs,4)) & AMASK;
				break;
		}
		buffer[offs & 15] = RDMEM( address + pgm_memory_base );
		if( (offs & 15) == 0 )
			fprintf(file, "%0*X: ", width, lshift((start + offs) & AMASK) );
		fprintf(file, "%02X", buffer[offs & 15] );
		if( (offs & 15) == 15 )
		{
			unsigned o;
			if( asciimode )
			{
				fputc( ' ', file );
				if( asciimode == 1 )
					for( o = 0; o < 16; o++ )
						fputc( trans_table[buffer[o]], file );
				else
					for( o = offs - 15; o <= offs; o++ )
						fputc( (buffer[o] < 32) ? '.' : buffer[o], file );
			}
			fprintf(file, "\n" );
			x = 0;
		}
		else
		if( (x += 2) == datasize )
		{
			if( (offs & 15) == 7 )
				fprintf(file, "-");
			else
				fprintf(file, " ");
			x = 0;
		}
	}
	fclose( file );

	edit_cmds_reset();
}

/**************************************************************************
 * cmd_save_to_file
 * Save binary image of a range of OP_ROM or OP_RAM
 **************************************************************************/
static void cmd_save_to_file( void )
{
	char *cmd = CMD;
	const char *filename;
	int length;
	FILE *file;
	unsigned start, end, offs;
	unsigned save_what;

	filename = get_file_name( &cmd, &length );
	if( !length )
	{
		win_msgbox( cur_col[E_ERROR], "SAVE arguments",
			"<filename> missing");
		edit_cmds_reset();
		return;
	}
	start = get_register_or_value( &cmd, &length );
	if( !length )
	{
		win_msgbox( cur_col[E_ERROR], "SAVE arguments",
			"<start> address missing");
		edit_cmds_reset();
		return;
	}
	end = get_register_or_value( &cmd, &length );
	if( !length )
	{
		win_msgbox( cur_col[E_ERROR], "SAVE arguments",
			"<end> address missing");
		edit_cmds_reset();
		return;
	}

	save_what = get_option_or_value( &cmd, &length, "OPCODES\0DATA\0");
	if( !length ) save_what = 0;	/* default to OP_ROM */

	file = fopen(filename, "wb");
	if( !file )
	{
		win_msgbox( cur_col[E_ERROR], "SAVE to file",
			"Could not create %s", filename);
		edit_cmds_reset();
		return;
	}

	if( save_what )
		save_what = PGM_MEMORY;

	for( offs = 0; offs + start <= end; offs++ )
	{
		fputc( RDMEM( ( ( start + offs ) & AMASK ) + save_what ), file );
	}

	fclose( file );

	edit_cmds_reset();
}

/**************************************************************************
 * cmd_edit_memory
 * Set one of the memory display window's start address and
 * switch to that window
 **************************************************************************/
static void cmd_edit_memory( void )
{
	char *cmd = CMD;
	unsigned which, address;
	int length;

	which = xtou( &cmd, &length );
	if( length )
	{
		which = (which - 1) % MAX_MEM;
		address = get_register_or_value( &cmd, &length );
		address = rshift(address) & AMASK;
		if( length )
		{
			DBGMEM[which].offset = address % DBGMEM[which].size;
			DBGMEM[which].base = address - DBGMEM[which].offset;
			DBGMEM[which].nibble = 0;
			dump_mem( which, 0 );
		}
		switch( which )
		{
		case 0: DBG.window = EDIT_MEM1; break;
		case 1: DBG.window = EDIT_MEM2; break;
		}
	}
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_search_memory
 * Search the memory for a sequence of bytes
 **************************************************************************/
static void cmd_search_memory(void)
{
	static UINT8 search_data[16];
	static int search_count = 0;
	UINT32 win = MAX_WINDOWS-3;
	unsigned which = (DBG.window == EDIT_MEM1) ? 0 : 1;
	unsigned w, h;
	unsigned shift, mask, val;
	const char *k;
	int i, offset, nibble;

	get_screen_size( &w, &h );
	win_create( win, 1, 2, 2, w-4, 2,
		cur_col[E_PROMPT], cur_col[E_FRAME], ' ',
		BORDER_TOP | BORDER_BOTTOM | BORDER_LEFT | BORDER_RIGHT );
	win_set_title( win, "Search byte(s) in memory");
	win_show( win );

	offset = 0;
	nibble = 0;

	do
	{
		win_set_curpos( win, 0, 0 );
		for( i = 0; i < search_count; i++ )
			win_printf( win, "%02X ", search_data[i] );
		win_erase_eol( win, ' ' );
		win_set_curpos( win, 0, 1 );
		for( i = 0; i < search_count; i++ )
			win_printf( win, "%c  ", search_data[i] < 32 ? '.' : search_data[i] );
		win_erase_eol( win, ' ' );

		win_set_curpos( win, offset * 3 + nibble, 0 );
		set_screen_curpos( win_get_cx_abs(win), win_get_cy_abs(win) );

		i = readkey();
		k = keyboard_name(i);

		shift = (1 - nibble) * 4;
		mask = ~(0xf << shift);

		if( strlen(k) == 1 )
		{
			switch( k[0] )
			{
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
			case '8': case '9': case 'A': case 'B':
			case 'C': case 'D': case 'E': case 'F':
				if( offset == 16 )
					break;
				if( offset == search_count )
					search_count++;
				val = k[0] - '0';
				if( val > 9 ) val -= 7;
				val <<= shift;
				/* now modify the register */
				search_data[offset] = (search_data[offset] & mask ) | val;
				i = KEYCODE_RIGHT;	/* advance to next nibble */
				break;
			}
		}

		switch( i )
		{
			case KEYCODE_DEL:
				if( offset < search_count )
				{
					for( i = offset; i < 15; i++ )
						search_data[i] = search_data[i+1];
					search_count--;
				}
				break;

			case KEYCODE_INSERT:
				if( search_count < 16 )
				{
					for( i = 15; i > offset; i-- )
						search_data[i] = search_data[i-1];
					search_data[offset] = 0;
					search_count++;
				}
				break;

			case KEYCODE_BACKSPACE:
				if( nibble > 0 || offset > 0 )
				{
					if( nibble > 0 )
					{
						nibble--;
					}
					else
					if( offset > 0 )
					{
						offset--;
						nibble = 1;
					}
					shift = (1 - nibble) * 4;
					mask = ~(0xf << shift);
					/* now modify the value */
					search_data[offset] = search_data[offset] & mask;
				}
				break;

			case KEYCODE_LEFT:
				if( nibble > 0 )
				{
					nibble--;
				}
				else
				if( offset > 0 )
				{
					offset--;
					nibble = 1;
				}
				break;

			case KEYCODE_RIGHT:
				if( offset < search_count )
				{
					if( nibble < 1 )
					{
						nibble++;
					}
					else
					{
						offset++;
						nibble = 0;
					}
				}
				break;

			case KEYCODE_HOME:
				offset = 0;
				nibble = 0;
				break;

			case KEYCODE_END:
				offset = search_count;
				nibble = 0;
				break;
		}
	} while( i != KEYCODE_ENTER && i != KEYCODE_ESC );

	if( i == KEYCODE_ENTER && search_count > 0 )
	{
		static char dbg_info[32+1];
		unsigned addr, start;

		start = (DBGMEM[which].base + DBGMEM[which].offset) & AMASK;

		for( addr = start + 1; addr != start; addr = (addr + 1) & AMASK )	// adopted from MAME 0.89
		{
			if( (addr & (AMASK >> 8)) == 0 )
			{
				win_set_title( win, "[%3.0f%%] %s/%s",
					100.0 * addr / (AMASK + 1),
					kilobyte(addr), kilobyte(AMASK + 1) );
			}
			for( i = 0; i < search_count; i++)
				if( RDMEM( addr+i ) != search_data[i] )
					break;
			if( i == search_count )
			{
				sprintf(dbg_info, "Found at address $%X", addr);
				dbg_info_once = dbg_info;
				DBGMEM[which].offset = addr % DBGMEM[which].size;
				DBGMEM[which].base = addr - DBGMEM[which].offset;
				win_close(win);
				dbg_update = 1;
				return;
			}
		}
		sprintf(dbg_info, "Not found");
		dbg_info_once = dbg_info;
	}
	win_close(win);
}

/**************************************************************************
 * cmd_fast
 * Hmm.. no idea ;)
 **************************************************************************/
static void cmd_fast( void )
{
	dbg_fast = 1;
	cmd_go_break();
}

/**************************************************************************
 * cmd_go_break
 * Let the emulation run and optionally set a breakpoint
 **************************************************************************/
static void cmd_go_break( void )
{
	char *cmd = CMD;
	unsigned brk;
	int length;

	brk = get_register_or_value( &cmd, &length );
	if( length )
		DBG.brk_temp = brk;

	dbg_update = 0;
	dbg_active = 0;

	osd_sound_enable(1);
	debugger_focus = 0;
}

/**************************************************************************
 * cmd_here
 * Set a temporary breakpoint at the cursor PC and let the emulation run
 **************************************************************************/
static void cmd_here( void )
{
	DBG.brk_temp = DBGDASM.pc_cur;

	dbg_update = 0;
	dbg_active = 0;

	osd_sound_enable(1);
	debugger_focus = 0;

	edit_cmds_reset();
}


#ifdef PINMAME
/**************************************************************************
 * cmd_jumpover
 * Set a temporary breakpoint at the PC directly after the cursor pc
 * and let the emulation run
 **************************************************************************/
static void cmd_jumpover( void )
{
	DBG.brk_temp = dasm_line( DBGDASM.pc_cur, 1 );

	dbg_update = 0;
	dbg_active = 0;

	osd_sound_enable(1);
	debugger_focus = 0;

	edit_cmds_reset();
}
/**************************************************************************
 * cmd_sound_enable
 * Allow sound to be enabled while debugger window is open
 **************************************************************************/
static void cmd_sound_enable( void )
{
	dbg_sound_disable = 0;

	osd_sound_enable(1);

	edit_cmds_reset();
}
/**************************************************************************
 * cmd_sound_disable
 * Allow sound to be enabled while debugger window is open
 **************************************************************************/
static void cmd_sound_disable( void )
{
	dbg_sound_disable = 1;

	osd_sound_enable(0);

	edit_cmds_reset();
}
#endif

/**************************************************************************
 * cmd_set_ignore
 * Ignore a CPU while debugging and tracing
 **************************************************************************/
static void cmd_set_ignore( void )
{
	char *cmd = CMD;
	unsigned cpunum;
	int i, length, already_ignored;

	cpunum = xtou( &cmd, &length );
	if( cpunum < total_cpu )
	{
		if( !dbg[cpunum].ignore )
		{
			for( i = 0, already_ignored = 0; i < total_cpu; i++ )
				if(dbg[i].ignore) ++already_ignored;
			if( already_ignored + 1 >= total_cpu )
			{
				win_msgbox( cur_col[E_ERROR], "Ignore CPU",
					"No, I won't do that! ;-)\nIgnoring all CPUs is a bad idea.");
				edit_cmds_reset();
				return;
			}
			dbg[cpunum].ignore = 1;
			if( cpunum == active_cpu )
				cmd_focus_next_cpu();
		}
	}
	else
	{
		win_msgbox( cur_col[E_ERROR], "Ignore CPU",
			"The selected CPU# is too high.\nOnly %d CPUs (0..%d) are used", total_cpu, total_cpu-1);
	}

	edit_cmds_reset();
}

/**************************************************************************
 * cmd_set_observe
 * Observe a CPU while debugging and tracing
 **************************************************************************/
static void cmd_set_observe( void )
{
	char *cmd = CMD;
	unsigned cpunum;
	int length;

	cpunum = xtou( &cmd, &length );
	if( cpunum < total_cpu )
	{
		dbg[cpunum].ignore = 0;
	}
	else
	{
		win_msgbox( cur_col[E_ERROR], "Observe CPU",
			"The selected CPU# is too high.\nOnly %d CPUs (0..%d) are used", total_cpu, total_cpu-1);
	}

	edit_cmds_reset();
}

/**************************************************************************
 * cmd_jump
 * Jump to the specified address in the disassembly window
 **************************************************************************/
static void cmd_jump( void )
{
	char *cmd = CMD;
	unsigned address;
	int length;

	address = get_register_or_value( &cmd, &length );
	if( length > 0 )
	{
		DBGDASM.pc_top = address;
		DBGDASM.pc_cur = DBGDASM.pc_top;
		DBGDASM.pc_end = dump_dasm( DBGDASM.pc_top );
	}
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_replace_register
 * Either change a register to a specified value, change to the
 * registers window to edit a specified or (if none given) the
 * first register
 **************************************************************************/
static void cmd_replace_register( void )
{
	char *cmd = CMD;
	unsigned regnum, address;
	int length;

	regnum = get_register_id( &cmd, &length );
	if( regnum > 0 )
	{
		address = get_register_or_value( &cmd, &length );
		if( length )
		{
			activecpu_set_reg( regnum, address );
			if( regnum > 1 )
				dump_regs();
			else
				/* Update in case PC changed */
				dbg_update = 1;
		}
		else
		{
			/* Edit the first register */
			for( DBGREGS.idx = 0; DBGREGS.idx < DBGREGS.count; DBGREGS.idx++ )
				if( DBGREGS.id[DBGREGS.idx] == regnum ) break;
			DBG.window = EDIT_REGS;
		}
	}
	else
	{
		/* Edit the first register */
		DBGREGS.idx = 0;
		DBG.window = EDIT_REGS;
	}
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_set_memory_mode
 * Set display mode of a memory window
 **************************************************************************/
static void cmd_set_memory_mode( void )
{
	char *cmd = CMD;
	unsigned which, mode;
	int length;

	which = dtou( &cmd, &length );
	if( length )
	{
		which = (which - 1) % 2;
		mode = get_option_or_value( &cmd, &length, "BYTE\0WORD\0DWORD\0" );
		if( !length ) mode = 0; /* default to BYTE */
		DBGMEM[which].mode = mode;
	}
	else
	{
		DBGMEM[0].mode = 0;
		DBGMEM[1].mode = 0;
	}
	edit_cmds_reset();
	dbg_update = 1;
}

/**************************************************************************
 * cmd_set_dasm_relative_jumps
 * Turn display of relative jumps as $+/-offset on or off
 **************************************************************************/
static void cmd_set_dasm_relative_jumps( void )
{
	char *cmd = CMD;

	dbg_dasm_relative_jumps = get_boolean( &cmd, NULL );

	edit_cmds_reset();
	dbg_update = 1;
}

/**************************************************************************
 * cmd_trace_to_file
 * Turn tracing to file on or off
 * If it is to be turned on, expect filename and optionally
 * a list of register names to dump
 **************************************************************************/
static void cmd_trace_to_file( void )
{
	char *cmd = CMD;
	const char *filename;
	UINT8 regs[MAX_REGS], regcnt = 0;
	int length;

	filename = get_file_name( &cmd, &length );

	if( !my_stricmp( filename, "OFF" ) )
	{
		trace_done();
	}
	else
	{
		while( *cmd )
		{
			regs[regcnt] = get_register_id( &cmd, &length );
			if( regs[ regcnt ] > 0 )
			{
				regcnt++;
			}
			else
			{
				break;
			}
		}
		regs[regcnt] = 0;

		trace_init( filename, regs );
	}

	edit_cmds_reset();
}

/**************************************************************************
 * cmd_dasm_up
 * Move cursor line to previous instruction
 **************************************************************************/
static void cmd_dasm_up( void )
{
	if ( (DBGDASM.pc_cur >= dasm_line( DBGDASM.pc_top, 1 ) ) &&
		 ((DBGDASM.pc_cur < DBGDASM.pc_end) || (DBGDASM.pc_end < DBGDASM.pc_top)) )
	{
		unsigned dasm_pc_1st = DBGDASM.pc_top;
		unsigned dasm_pc_2nd = DBGDASM.pc_top;
		while( dasm_pc_2nd != DBGDASM.pc_end )
		{
			dasm_pc_2nd = dasm_line( dasm_pc_1st, 1 );

			if( dasm_pc_2nd == DBGDASM.pc_cur )
			{
				DBGDASM.pc_cur = dasm_pc_1st;
				dasm_pc_2nd = DBGDASM.pc_end;
			}
			else
			{
				dasm_pc_1st = dasm_pc_2nd;
			}
		}
	}
	else
	if( DBGDASM.pc_top > 0 )
	{
		/*
		 * Try to find the previous instruction by searching from the
		 * longest instruction length towards the current address.
		 * If we can't find one then just go back one byte,
		 * which means that a previous guess was wrong.
		 */
		unsigned dasm_pc_tmp = rshift(DBGDASM.pc_top - lshift(INSTL)) & AMASK;
		int i;
		for( i = 0; i < INSTL; i += ALIGN )
		{
			if( dasm_line( lshift(dasm_pc_tmp), 1 ) == DBGDASM.pc_top )
				break;
			dasm_pc_tmp += ALIGN;
		}
		dasm_pc_tmp = lshift(dasm_pc_tmp);
		if( dasm_pc_tmp == DBGDASM.pc_top )
			dasm_pc_tmp -= ALIGN;
		DBGDASM.pc_cur = dasm_pc_tmp;
		if( DBGDASM.pc_cur < DBGDASM.pc_top )
			DBGDASM.pc_top = DBGDASM.pc_cur;
	}
	DBGDASM.pc_end = dump_dasm( DBGDASM.pc_top );
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_dasm_down
 * Move cursor line to next instruction
 **************************************************************************/
static void cmd_dasm_down( void )
{
	DBGDASM.pc_cur = dasm_line( DBGDASM.pc_cur, 1 );
	if( DBGDASM.pc_cur >= DBGDASM.pc_end )
		DBGDASM.pc_top = dasm_line( DBGDASM.pc_top, 1 );
	DBGDASM.pc_end = dump_dasm( DBGDASM.pc_top );
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_dasm_page_up
 * Disassemble previous page
 **************************************************************************/
static void cmd_dasm_page_up( void )
{
	UINT32 i;
	/*
	 * This uses a 'rolling window' of start addresses to work out
	 * the best address to use to generate the previous pagefull of
	 * disassembly - CM 980428
	 */
	if( DBGDASM.pc_top > 0 )
	{
		unsigned dasm_pc_row[50];	/* needs to be > max windows height */
		unsigned h = win_get_h(WIN_DASM(active_cpu));
		unsigned dasm_pc_tmp = lshift((rshift(DBGDASM.pc_top) - h * INSTL) & AMASK);

		if( dasm_pc_tmp > DBGDASM.pc_top )
		{
			DBGDASM.pc_top = 0;
		}
		else
		{
			for( i= 0; dasm_pc_tmp < DBGDASM.pc_top; i++ )
			{
				dasm_pc_row[i % h] = dasm_pc_tmp;
				dasm_pc_tmp = dasm_line( dasm_pc_tmp, 1 );
			}

			/*
			 * If this ever happens, it's because our
			 * max_inst_len member is too small for the CPU
			 */
			if( i < h )
			{
				dasm_pc_tmp = dasm_pc_row[0];
				win_msgbox(cur_col[E_ERROR], "DBGDASM page up",
					"Increase cpu_intf[].max_inst_len? Line = %d\n", i);
			}
			else
			{
				DBGDASM.pc_top = dasm_pc_row[(i + 1) % h];
			}
		}
	}
	DBGDASM.pc_cur = DBGDASM.pc_top;
	DBGDASM.pc_end = dump_dasm( DBGDASM.pc_top );
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_dasm_page_down
 * Disassemble next page
 **************************************************************************/
static void cmd_dasm_page_down( void )
{
	unsigned h = win_get_h(WIN_DASM(active_cpu));

	DBGDASM.pc_top = dasm_line( DBGDASM.pc_top, h );
	DBGDASM.pc_cur = dasm_line( DBGDASM.pc_cur, h );
	DBGDASM.pc_end = dump_dasm( DBGDASM.pc_top );
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_dasm_home
 * Disassemble first page
 **************************************************************************/
static void cmd_dasm_home( void )
{
	DBGDASM.pc_cur = DBGDASM.pc_top = 0;
	DBGDASM.pc_end = dump_dasm( DBGDASM.pc_top );
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_dasm_end
 * Disassemble last page
 **************************************************************************/
static void cmd_dasm_end( void )
{
	unsigned h = win_get_h(WIN_DASM(active_cpu));
	unsigned tmp_address = lshift(AMASK - h * INSTL + 1);
	unsigned end_address;
	for( ; ; )
	{
		end_address = dasm_line( tmp_address, h );
		if( end_address < tmp_address )
			break;
		tmp_address += ALIGN;
	}
	DBGDASM.pc_top = tmp_address;
	DBGDASM.pc_cur = dasm_line( DBGDASM.pc_top, h - 1 );
	DBGDASM.pc_end = dump_dasm( DBGDASM.pc_top );
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_brk_exec_toggle
 * Toggle execution break point at cursor line
 **************************************************************************/
static void cmd_brk_exec_toggle( void )
{
	if( DBG.brk_exec == INVALID )
	{
		DBG.brk_exec = DBGDASM.pc_cur;
		DBG.brk_exec_times = 0;
		DBG.brk_exec_reset = 0;
		dbg_update = 1;
	}
	else
	if( DBG.brk_exec == DBGDASM.pc_cur )
	{
		DBG.brk_exec = INVALID;
		DBG.brk_exec_times = 0;
		DBG.brk_exec_reset = 0;
		dbg_update = 1;
	}
	else
	{
		win_msgbox( cur_col[E_PROMPT], "Breakpoint",
			"Cleared execution break point at $%X", DBG.brk_exec );
		DBG.brk_exec = INVALID;
	}
}

/**************************************************************************
 * cmd_dasm_hist_follow
 * Follow the current code or data reference
 **************************************************************************/
static void cmd_dasm_hist_follow( void )
{
	unsigned i, address, access = EA_NONE;

	address = INVALID;
	DBGDASM.dst_ea_value = INVALID;
	DBGDASM.src_ea_value = INVALID;

	dasm_line( DBGDASM.pc_cur, 1 );

	if( DBGDASM.src_ea_value != INVALID )
	{
		access = DBGDASM.src_ea_access;
		address = DBGDASM.src_ea_value;
	}
	if( DBGDASM.dst_ea_value != INVALID )
	{
		access = DBGDASM.dst_ea_access;
		address = DBGDASM.dst_ea_value;
	}
	if( address != INVALID )
	{
		if( DBG.hist_cnt < MAX_HIST )
		{
			i = DBG.hist_cnt;
			/* Save some current values */
			DBG.hist[i].dasm_top = DBGDASM.pc_top;
			DBG.hist[i].dasm_cur = DBGDASM.pc_cur;
			DBG.hist[i].mem1_base = DBGMEM[0].base;
			DBG.hist[i].mem1_offset = DBGMEM[0].offset;
			DBG.hist[i].mem1_nibble = DBGMEM[0].nibble;
			if( access == EA_ABS_PC || access == EA_REL_PC )
			{
				DBGDASM.pc_top = address;
				DBGDASM.pc_cur = address;
				DBGDASM.pc_end = dump_dasm( DBGDASM.pc_top );
				DBG.hist_cnt++;
			}
			else
			if( access == EA_MEM_RD || access == EA_MEM_WR || access == EA_MEM_RDWR ||
				access == EA_ZPG_RD || access == EA_ZPG_WR || access == EA_ZPG_RDWR )
			{
				DBGMEM[0].offset = address	% DBGMEM[0].size;
				DBGMEM[0].base = address - DBGMEM[0].offset;
				DBGMEM[0].nibble = 0;
				dump_mem( 0, 0 );
				DBG.hist_cnt++;
			}
		}
	}
}

/**************************************************************************
 * cmd_dasm_hist_back
 * Back to the previous point in the 'follow history'
 **************************************************************************/
static void cmd_dasm_hist_back( void )
{
	unsigned i;
	if( DBG.hist_cnt > 0)
	{
		i = --DBG.hist_cnt;
		DBGDASM.pc_top = DBG.hist[i].dasm_top;
		DBGDASM.pc_cur = DBG.hist[i].dasm_cur;
		DBGDASM.pc_end = dump_dasm( DBGDASM.pc_top );
		DBGMEM[0].base = DBG.hist[i].mem1_base;
		DBGMEM[0].offset = DBG.hist[i].mem1_offset;
		DBGMEM[0].nibble = DBG.hist[i].mem1_nibble;
		dump_mem( 0, 0 );
	}
}

/**************************************************************************
 * cmd_brk_exec_toggle
 * Toggle data break point at memory location
 **************************************************************************/
static void cmd_brk_data_toggle( void )
{
	int which = DBG.window == EDIT_MEM1 ? 0 : 1;

	if( DBG.brk_data == INVALID )
	{
		unsigned data;
		DBG.brk_data = DBGMEM[which].address;
		data = RDMEM(DBG.brk_data);
		DBG.brk_data_oldval = data;
		DBG.brk_data_newval = INVALID;
	}
	else
	if( DBG.brk_data == DBGMEM[which].address )
	{
		DBG.brk_data = INVALID;
		DBG.brk_data_oldval = INVALID;
		DBG.brk_data_newval = INVALID;
	}
	else
	{
		win_msgbox( cur_col[E_PROMPT], "Data watchpoint",
			"Cleared data watch point at $%X", DBG.brk_data );
		DBG.brk_data = INVALID;
	}
	dbg_update = 1;
	edit_cmds_reset();
}

/**************************************************************************
 * cmd_toggle_scanlines
 * Toggles the display of scanlines in the display
 **************************************************************************/
static void cmd_toggle_scanlines( void )
{
	dbg_show_scanline = !dbg_show_scanline;
}

/**************************************************************************
 * cmd_run_to_cursor
 * Set temporary break point at cursor line and go
 **************************************************************************/
static void cmd_run_to_cursor( void )
{
	DBG.brk_temp = DBGDASM.pc_cur;

	edit_cmds_reset();

	cmd_go();
}

/**************************************************************************
 * cmd_focus_next_cpu
 * Switch focus to the next CPU
 **************************************************************************/
static void cmd_focus_next_cpu( void )
{
	if( total_cpu > 1 )
	{
		win_set_title(WIN_CMDS(active_cpu), "CPU #%d yield", active_cpu);
		cpu_yield();
		dbg_update = 1;
		dbg_step = 1;
	}

	edit_cmds_reset();
}

/*******************************************************
 * cmd_go
 * Let the game run
 **************************************************************************/
static void cmd_go( void )
{
	debug_key_pressed = 0;

	dbg_update = 0;
	dbg_active = 0;

	edit_cmds_reset();

	osd_sound_enable(1);
	debugger_focus = 0;
}

/**************************************************************************
 * cmd_set_key_repeat
 * Set the keyboard repeat rate (will decrease to 1/frame)
 **************************************************************************/
static void cmd_set_key_repeat( void )
{
	char *cmd = CMD;

	dbg_key_repeat = dtou( &cmd, NULL );
	if( dbg_key_repeat == 0 )
		dbg_key_repeat = Machine->drv->frames_per_second / 15;

	edit_cmds_reset();
	dbg_update = 1;
}

/**************************************************************************
 * cmd_set_dasm_case
 * Set the case style for the disassembly window
 * "DEFAULT", "LOWER" or "UPPER" are recognized comparing the first letter
 **************************************************************************/
static void cmd_set_dasm_case( void )
{
	char *cmd = CMD;

	if( toupper(cmd[0]) == 'D' )
		dbg_dasm_case = 0;
	else
	if( toupper(cmd[0]) == 'L' )
		dbg_dasm_case = 1;
	else
	if( toupper(cmd[0]) == 'U' )
		dbg_dasm_case = 2;
	else
		dbg_dasm_case = xtou( &cmd, NULL );

	edit_cmds_reset();
	dbg_update = 1;
}

/**************************************************************************
 * cmd_set_mem_squeezed
 * Set allow squeezed memory display
 **************************************************************************/
static void cmd_set_mem_squeezed( void )
{
	char *cmd = CMD;

	dbg_mem_squeezed = get_boolean( &cmd, NULL );

	edit_cmds_reset();
	dbg_update = 1;
}

/**************************************************************************
 * cmd_set_dasm_opcodes
 * Set allow squeezed memory display
 **************************************************************************/
static void cmd_set_dasm_opcodes( void )
{
	char *cmd = CMD;
	UINT32 win = WIN_DASM(active_cpu);
	UINT32 w = win_get_w( win );
	UINT32 dw = (INSTL / ALIGN) * (ALIGN * 2 + 1);
	int state;

	state = get_boolean( &cmd, NULL );

	if( dbg_dasm_opcodes != state )
	{
		dbg_dasm_opcodes = state;
		if( state )
		{
			win_set_prio( win, 0 );
			win_set_w( win, w + dw	);
		}
		else
		{
			win_set_prio( win, 1 );
			win_set_w( win, w - dw );
		}
		DBGDASM.pc_end = dump_dasm( DBGDASM.pc_top );
	}

	edit_cmds_reset();
	dbg_update = 1;
}

static void mame_debug_reset_statics( void )
{
	/* Reset the statics */
	memset( &dbg, 0, sizeof(dbg) );

	active_cpu = INVALID;
	previous_active_cpu = INVALID;
	total_cpu = 0;
	cputype = 0;

	dbg_fast = 0;
	dbg_step = 0;
	dbg_trace = 0;
	dbg_update = 0;
	dbg_update_cur = 0;
	dbg_active = 0;
	dbg_mem_squeezed = 0;
	dbg_dasm_opcodes = 0;
	dbg_dasm_case = 0;
	dbg_dasm_relative_jumps = 0;
	dbg_info_once = NULL;
}


/**************************************************************************
 *	mame_debug_init
 *	This function is called from cpu_run to startup the debugger
 **************************************************************************/
void mame_debug_init(void)
{
	char filename[127+1];
	FILE *file;

	mame_debug_reset_statics();
	debugger_focus = 1;

	total_cpu = cpu_gettotalcpu();

	for( active_cpu = 0; active_cpu < total_cpu; active_cpu++ )
	{
		DBG.window = EDIT_CMDS;
		DBG.brk_exec = INVALID;
		DBG.brk_exec_times = 0;
		DBG.brk_exec_reset = 0;
		DBG.brk_data = INVALID;
		DBG.brk_data_oldval = INVALID;
		DBG.brk_data_newval = INVALID;
		DBG.brk_regs = INVALID;
		DBG.brk_regs_oldval = INVALID;
		DBG.brk_regs_newval = INVALID;
		DBG.brk_regs_mask = 0xffffffff;
		DBG.brk_temp = INVALID;
#if defined(PINMAME) && defined(DBG_BPR)
		DBG.bprstart = INVALID;
		DBG.bprnext = 0;
#endif /* DBG_BPR */
		DBGMEM[0].base = 0x0000;
		DBGMEM[1].base = 1 << (ABITS / 2);
		switch( cpunum_align_unit(active_cpu) )
		{
			case 1: DBGMEM[0].mode = DBGMEM[1].mode = MODE_HEX_UINT8;  break;
			case 2: DBGMEM[0].mode = DBGMEM[1].mode = MODE_HEX_UINT16; break;
			case 4: DBGMEM[0].mode = DBGMEM[1].mode = MODE_HEX_UINT32; break;
		}
	}

	/* set keyboard repeat rate based on the game's frame rate */
	dbg_key_repeat = Machine->drv->frames_per_second / 15;

	/* create windows for the active CPU */
	dbg_open_windows();

	/* See if there is an existing global mamedbg config file */
	strcpy( filename, "mamedbg.cfg" );
	file = fopen( filename, "r" );
	if( file )
	{
		char *cmdline = CMD, *p;
		int cmd;

		while( !feof(file) )
		{
			fgets( cmdline, 80, file );
			cmdline[80] = '\0';
			if( *cmdline == ';' || *cmdline == '#' )
				continue;
			p = strchr( cmdline, '\r' );
			if( p ) *p = '\0';
			p = strchr( cmdline, '\n' );
			if( p ) *p = '\0';
			/* Make it all upper case */
			strcpy( cmdline, upper(cmdline) );
			cmd = edit_cmds_parse( cmdline );
			if( cmd != INVALID && commands[cmd].function )
				(*commands[cmd].function)();
		}
		fclose(file);
	}
	for( active_cpu = 0; active_cpu < total_cpu; active_cpu++ )
	{
		/* See if there is an existing startup file <game>.cf<cpunum> */
		sprintf( filename, "%s.cf%d", Machine->gamedrv->name, active_cpu );
		file = fopen( filename, "r" );
		if( file )
		{
			char *cmdline = CMD, *p;
			int cmd;

			while( !feof(file) )
			{
				fgets( cmdline, 80, file );
				cmdline[80] = '\0';
				if( *cmdline == ';' || *cmdline == '#' )
					continue;
				p = strchr( cmdline, '\r' );
				if( p ) *p = '\0';
				p = strchr( cmdline, '\n' );
				if( p ) *p = '\0';
				/* Make it all upper case */
				strcpy( cmdline, upper(cmdline) );
				cmd = edit_cmds_parse( cmdline );
				if( cmd != INVALID && commands[cmd].function )
					(*commands[cmd].function)();
			}
			fclose(file);
		}
	}

	debug_key_pressed = 1;

	first_time = 1;
}

/**************************************************************************
 *	mame_debug_exit
 *	This function is called from cpu_run to shutdown the debugger
 **************************************************************************/
void mame_debug_exit(void)
{
#ifdef PINMAME
/* CODELIST */
	codelist_done();
#endif /* PINMAME */
	dbg_close_windows();
	mame_debug_reset_statics();
}


#ifdef PINMAME
/* CODELIST */
/**************************************************************************
 * cmd_codelist_to_file
 * Turn codelist to file on or off
 * If it is to be turned on, expect filename
 **************************************************************************/
static void cmd_codelist_to_file( void )
{
	char *cmd = CMD;
	const char *filename;
	int length;

	filename = get_file_name( &cmd, &length );

	if( !my_stricmp( filename, "OFF" ) )
	{
		codelist_done();
	}
	else
	{
		codelist_init( filename );
	}

	edit_cmds_reset();
}

/**************************************************************************
 * codelist_init
 * Creates codelist output files for all CPUs
 **************************************************************************/
static void codelist_init( const char *given_filename )
{
	extern int activecpu;	// needed for ABITS and AMASK
	int prevcpu;
	char *filename;

	char name[100];

	if( codelist_on )
		return;

	// if no filename given, default to game name
	filename = (char *)given_filename;
	if ( !filename || filename[0] == 0 )
	{
		filename = (char *)Machine->gamedrv->name;
	}

	prevcpu = activecpu;
	for( tracecpu = 0; tracecpu < total_cpu; tracecpu++ )
	{
		sprintf( name, "%s.codelist.cpu%d", filename, tracecpu );
		TRACE.codelistfile = fopen(name,"w");
		TRACE.codelist = NULL;

		activecpu = tracecpu;	// needed for ABITS and AMASK
		TRACE.addr_width = (ABITS + 3) / 4;
		TRACE.addr_mask = AMASK;
	}
	activecpu = prevcpu;

	tracecpu = active_cpu;
	codelist_on = 1;
}

/**************************************************************************
 * codelist_done
 * Closes the codelist output files
 **************************************************************************/
void codelist_done(void)
{
	struct s_codelist	*codelist, *prevlist;
	struct s_codeblock	*codeblock, *prevblock;
	static char buffer[127+1];
	char *dst, *dst2;
	UINT32	start, end, size;

	if( !codelist_on )
		return;

	for( tracecpu = 0; tracecpu < total_cpu; tracecpu++ )
	{
		if( TRACE.codelistfile )
		{
			// list banks
			codelist = TRACE.codelist;
			size = 0;
			dst = buffer;
			dst += sprintf( dst, "banks:" );
			while ( codelist )
			{
				if ( codelist->bankid != FAKE_BANKID )
				{
					dst += sprintf( dst, " 0x%02X", codelist->bankid );
				}
				else
				{
					dst += sprintf( dst, " STATIC" );
				}

				size++;
				if ( size == 8 )
				{
					fprintf( TRACE.codelistfile, "%s\n", buffer );
					size = 0;
					dst = buffer;
					dst += sprintf( dst, "banks:" );
				}

				codelist = codelist->next;
			}
			if ( size > 0 )
			{
				fprintf( TRACE.codelistfile, "%s\n", buffer );
			}
			sprintf( buffer, "---" );
			fprintf( TRACE.codelistfile, "%s\n", buffer );

			// list codeblocks of banks
			codelist = TRACE.codelist;
			while ( codelist )
			{
				dst = buffer;
				if ( codelist->bankid != FAKE_BANKID )
				{
					dst += sprintf( dst, "0x%02X", codelist->bankid );
				}
				else
				{
					dst += sprintf( dst, "STATIC" );
				}
				dst += sprintf( dst, ": code\t0x" );

				codeblock = codelist->blocks;
				while ( codeblock )
				{
					start = codeblock->startAddr & TRACE.addr_mask;
					end = ( codeblock->afterAddr - 1 ) & TRACE.addr_mask;
					size = codeblock->afterAddr - codeblock->startAddr;

					dst2 = dst;
					dst2 += sprintf( dst2, "%0*X", TRACE.addr_width, start );
					dst2 += sprintf( dst2, "\t\t; up to 0x%0*X", TRACE.addr_width, end );
					dst2 += sprintf( dst2, ", size 0x%0*X", TRACE.addr_width, size );
					fprintf( TRACE.codelistfile, "%s\n", buffer );

					prevblock = codeblock;
					codeblock = codeblock->next;
					free ( prevblock );
				}

				sprintf( buffer, "---" );
				fprintf( TRACE.codelistfile, "%s\n", buffer );

				prevlist = codelist;
				codelist = codelist->next;
				free ( prevlist );
			}
			TRACE.codelist = NULL;

			fclose( TRACE.codelistfile );
			TRACE.codelistfile = NULL;
		}
	}

	tracecpu = active_cpu;
	codelist_on = 0;
}

/**************************************************************************
 * codelist_select
 * Codelist collecting
 **************************************************************************/
static void codelist_select( void )
{
	UINT32 bankid;
	struct s_codelist	*codelist, *prevlist;

	UINT32 pc;
	struct s_codeblock	*codeblock, *prevblock;
	UINT32 size, afteraddr;
	static char buffer[127+1];

	if( active_cpu < total_cpu )
		tracecpu = active_cpu;

	if( !codelist_on || !TRACE.codelistfile )
	{
		return;
	}

	// Find a matching codelist for the bank id
	bankid = FAKE_BANKID;
	if ( opcode_entry >= STATIC_BANK1 && opcode_entry <= STATIC_BANKMAX )
	{
		bankid = cpu_bankid[opcode_entry];
	}

	prevlist = NULL;
	codelist = TRACE.codelist;
	while ( codelist )
	{
		if ( codelist->bankid == bankid )	// found match
		{
			break;
		}
		if ( codelist->bankid > bankid )	// not found in sorted list
		{
			codelist = NULL;
			break;
		}

		prevlist = codelist;
		codelist = codelist->next;
	}
	if ( !codelist )	// not found, create new one
	{
		codelist = malloc ( sizeof (struct s_codelist) );

		codelist->bankid = bankid;
		codelist->blocks = NULL;

		// link into list
		if ( prevlist )
		{
			codelist->next = prevlist->next;
			prevlist->next = codelist;
		}
		else
		{
			codelist->next = TRACE.codelist;
			TRACE.codelist = codelist;
		}
	}

	// Find a matching codeblock for the PC
	pc = activecpu_get_pc();

	prevblock = NULL;
	codeblock = codelist->blocks;
	while ( codeblock )
	{
		if ( codeblock->startAddr <= pc && codeblock->afterAddr > pc )	// already recorded
		{
			return;	// nothing to do, exit routine
		}

		if ( codeblock->afterAddr == pc )	// found match/extendable at the end
		{
			break;
		}

		if ( codeblock->startAddr > pc )	// not found in sorted list, maybe extendable at the start
		{
			break;
		}

		prevblock = codeblock;
		codeblock = codeblock->next;
	}

	size = activecpu_dasm( buffer, pc );
	afteraddr = pc + size;

	if ( codeblock && codeblock->startAddr == afteraddr )	// extendable at the start
	{
		codeblock->startAddr = pc;
	}
	else if ( codeblock && codeblock->afterAddr == pc )	// extendable at the end
	{
		struct s_codeblock	*nextblock;

		codeblock->afterAddr = afteraddr;

		// check if codeblock can be merged with next one
		nextblock = codeblock->next;
		if ( nextblock && nextblock->startAddr == codeblock->afterAddr )
		{
			codeblock->afterAddr = nextblock->afterAddr;
			codeblock->next = nextblock->next;

			free ( nextblock );
		}
	}
	else	// really not found, create new codeblock
	{
		codeblock = malloc ( sizeof (struct s_codeblock) );

		codeblock->startAddr = pc;
		codeblock->afterAddr = afteraddr;

		// link into list
		if ( prevblock )
		{
			codeblock->next = prevblock->next;
			prevblock->next = codeblock;
		}
		else
		{
			codeblock->next = codelist->blocks;
			codelist->blocks = codeblock;
		}
	}
}
#endif /* PINMAME */

#endif
