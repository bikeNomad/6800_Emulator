/***************************************************************************

	memory.h

	Functions which handle the CPU memory and I/O port access.

***************************************************************************/

#ifndef _MEMORY_H
#define _MEMORY_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "osd_cpu.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Versions of GNU C earlier that 2.7 appear to have problems with the
 * __attribute__ definition of UNUSEDARG, so we act as if it was not a
 * GNU compiler.
 */

#ifdef __GNUC__
#if (__GNUC__ < 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ <= 7))
#define UNUSEDARG
#else
#define UNUSEDARG __attribute__((__unused__))
#endif
#else
#define UNUSEDARG
#endif



/*
 * Use __builtin_expect on GNU C 3.0 and above
 */
#ifdef __GNUC__
#if (__GNUC__ < 3)
#define UNEXPECTED(exp)	(exp)
#else
#define UNEXPECTED(exp)	 __builtin_expect((exp), 0)
#endif
#else
#define UNEXPECTED(exp)	(exp)
#endif



/***************************************************************************

	Parameters

***************************************************************************/

#ifdef MAME_DEBUG
#define CPUREADOP_SAFETY_NONE		0
#define CPUREADOP_SAFETY_PARTIAL	0
#define CPUREADOP_SAFETY_FULL		1
#elif defined(MESS)
#define CPUREADOP_SAFETY_NONE		0
#define CPUREADOP_SAFETY_PARTIAL	1
#define CPUREADOP_SAFETY_FULL		0
#else
#define CPUREADOP_SAFETY_NONE		1
#define CPUREADOP_SAFETY_PARTIAL	0
#define CPUREADOP_SAFETY_FULL		0
#endif

typedef void genf(void);


/***************************************************************************

	Basic type definitions

	These types are used for memory handlers.

***************************************************************************/

/* ----- typedefs for data and offset types ----- */
typedef UINT8			data8_t;
typedef UINT16			data16_t;
typedef UINT32			data32_t;
typedef UINT32			offs_t;

/* ----- typedefs for the various common memory/port handlers ----- */
typedef data8_t			(*read8_handler)  (UNUSEDARG offs_t offset);
typedef void			(*write8_handler) (UNUSEDARG offs_t offset, UNUSEDARG data8_t data);
typedef data16_t		(*read16_handler) (UNUSEDARG offs_t offset, UNUSEDARG data16_t mem_mask);
typedef void			(*write16_handler)(UNUSEDARG offs_t offset, UNUSEDARG data16_t data, UNUSEDARG data16_t mem_mask);
typedef data32_t		(*read32_handler) (UNUSEDARG offs_t offset, UNUSEDARG data32_t mem_mask);
typedef void			(*write32_handler)(UNUSEDARG offs_t offset, UNUSEDARG data32_t data, UNUSEDARG data32_t mem_mask);
typedef offs_t			(*opbase_handler) (UNUSEDARG offs_t address);

/* ----- typedefs for the various common memory handlers ----- */
typedef read8_handler	mem_read_handler;
typedef write8_handler	mem_write_handler;
typedef read16_handler	mem_read16_handler;
typedef write16_handler	mem_write16_handler;
typedef read32_handler	mem_read32_handler;
typedef write32_handler	mem_write32_handler;

/* ----- typedefs for the various common port handlers ----- */
typedef read8_handler	port_read_handler;
typedef write8_handler	port_write_handler;
typedef read16_handler	port_read16_handler;
typedef write16_handler	port_write16_handler;
typedef read32_handler	port_read32_handler;
typedef write32_handler	port_write32_handler;

/* ----- typedefs for externally allocated memory ----- */
struct ExtMemory
{
	offs_t 			start, end;
	UINT8			region;
    UINT8 *			data;
};



/***************************************************************************

	Basic macros

***************************************************************************/

/* ----- macros for declaring the various common memory/port handlers ----- */
#define READ_HANDLER(name) 		data8_t  name(UNUSEDARG offs_t offset)
#define WRITE_HANDLER(name) 	void     name(UNUSEDARG offs_t offset, UNUSEDARG data8_t data)
#define READ16_HANDLER(name)	data16_t name(UNUSEDARG offs_t offset, UNUSEDARG data16_t mem_mask)
#define WRITE16_HANDLER(name)	void     name(UNUSEDARG offs_t offset, UNUSEDARG data16_t data, UNUSEDARG data16_t mem_mask)
#define READ32_HANDLER(name)	data32_t name(UNUSEDARG offs_t offset, UNUSEDARG data32_t mem_mask)
#define WRITE32_HANDLER(name)	void     name(UNUSEDARG offs_t offset, UNUSEDARG data32_t data, UNUSEDARG data32_t mem_mask)
#define OPBASE_HANDLER(name)	offs_t   name(UNUSEDARG offs_t address)

/* ----- macros for accessing bytes and words within larger chunks ----- */
#ifdef LSB_FIRST
	#define BYTE_XOR_BE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
	#define BYTE_XOR_LE(a)  	(a)
	#define BYTE4_XOR_BE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
	#define BYTE4_XOR_LE(a) 	(a)
	#define WORD_XOR_BE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
	#define WORD_XOR_LE(a)  	(a)
#else
	#define BYTE_XOR_BE(a)  	(a)
	#define BYTE_XOR_LE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
	#define BYTE4_XOR_BE(a) 	(a)
	#define BYTE4_XOR_LE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
	#define WORD_XOR_BE(a)  	(a)
	#define WORD_XOR_LE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
#endif



/***************************************************************************

	Constants for static entries in port read/write arrays

***************************************************************************/

/* 8-bit port reads */
#define IORP_NOP				((port_read_handler)STATIC_NOP)

/* 8-bit port writes */
#define IOWP_NOP				((port_write_handler)STATIC_NOP)

/* 16-bit port reads */
#define IORP16_NOP				((port_read16_handler)STATIC_NOP)

/* 16-bit port writes */
#define IOWP16_NOP				((port_write16_handler)STATIC_NOP)

/* 32-bit port reads */
#define IORP32_NOP				((port_read32_handler)STATIC_NOP)

/* 32-bit port writes */
#define IOWP32_NOP				((port_write32_handler)STATIC_NOP)



/***************************************************************************

	Memory/port array type definitions

	Note that the memory hooks are not passed the actual memory address
	where the operation takes place, but the offset from the beginning
	of the block they are assigned to. This makes handling of mirror
	addresses easier, and makes the handlers a bit more "object oriented".
	If you handler needs to read/write the main memory area, provide a
	"base" pointer: it will be initialized by the main engine to point to
	the beginning of the memory block assigned to the handler. You may
	also provided a pointer to "size": it will be set to the length of
	the memory area processed by the handler.

***************************************************************************/

/* ----- structs for memory read arrays ----- */
struct Memory_ReadAddress
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	mem_read_handler 	handler;		/* handler callback */
};

struct Memory_ReadAddress16
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	mem_read16_handler 	handler;		/* handler callback */
};

struct Memory_ReadAddress32
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	mem_read32_handler	handler;		/* handler callback */
};

/* ----- structs for memory write arrays ----- */
struct Memory_WriteAddress
{
    offs_t				start, end;		/* start, end addresses, inclusive */
	mem_write_handler	handler;		/* handler callback */
	data8_t **			base;			/* receives pointer to memory (optional) */
    size_t *			size;			/* receives size of memory in bytes (optional) */
};

struct Memory_WriteAddress16
{
    offs_t				start, end;		/* start, end addresses, inclusive */
	mem_write16_handler handler;		/* handler callback */
	data16_t **			base;			/* receives pointer to memory (optional) */
    size_t *			size;			/* receives size of memory in bytes (optional) */
};

struct Memory_WriteAddress32
{
    offs_t				start, end;		/* start, end addresses, inclusive */
	mem_write32_handler handler;		/* handler callback */
	data32_t **			base;			/* receives pointer to memory (optional) */
	size_t *			size;			/* receives size of memory in bytes (optional) */
};

/* ----- structs for port read arrays ----- */
struct IO_ReadPort
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_read_handler 	handler;		/* handler callback */
};

struct IO_ReadPort16
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_read16_handler	handler;		/* handler callback */
};

struct IO_ReadPort32
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_read32_handler	handler;		/* handler callback */
};

/* ----- structs for port write arrays ----- */
struct IO_WritePort
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_write_handler	handler;		/* handler callback */
};

struct IO_WritePort16
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_write16_handler handler;		/* handler callback */
};

struct IO_WritePort32
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_write32_handler handler;		/* handler callback */
};




/***************************************************************************

	Macros to help declare handlers for core readmem/writemem routines

***************************************************************************/

/* ----- for declaring 8-bit handlers ----- */
#define DECLARE_HANDLERS_8BIT(type, abits) \
data8_t  cpu_read##type##abits             (offs_t offset);					\
void     cpu_write##type##abits            (offs_t offset, data8_t data);

/* ----- for declaring 16-bit bigendian handlers ----- */
#define DECLARE_HANDLERS_16BIT_BE(type, abits) \
data8_t  cpu_read##type##abits##bew        (offs_t offset);					\
data16_t cpu_read##type##abits##bew_word   (offs_t offset);					\
void     cpu_write##type##abits##bew       (offs_t offset, data8_t data);	\
void     cpu_write##type##abits##bew_word  (offs_t offset, data16_t data);

/* ----- for declaring 16-bit littleendian handlers ----- */
#define DECLARE_HANDLERS_16BIT_LE(type, abits) \
data8_t  cpu_read##type##abits##lew        (offs_t offset);					\
data16_t cpu_read##type##abits##lew_word   (offs_t offset);					\
void     cpu_write##type##abits##lew       (offs_t offset, data8_t data);	\
void     cpu_write##type##abits##lew_word  (offs_t offset, data16_t data);

/* ----- for declaring 32-bit bigendian handlers ----- */
#define DECLARE_HANDLERS_32BIT_BE(type, abits) \
data8_t  cpu_read##type##abits##bedw       (offs_t offset);					\
data16_t cpu_read##type##abits##bedw_word  (offs_t offset);					\
data32_t cpu_read##type##abits##bedw_dword (offs_t offset);					\
void     cpu_write##type##abits##bedw      (offs_t offset, data8_t data);	\
void     cpu_write##type##abits##bedw_word (offs_t offset, data16_t data);	\
void     cpu_write##type##abits##bedw_dword(offs_t offset, data32_t data);

/* ----- for declaring 32-bit littleendian handlers ----- */
#define DECLARE_HANDLERS_32BIT_LE(type, abits) \
data8_t  cpu_read##type##abits##ledw       (offs_t offset);					\
data16_t cpu_read##type##abits##ledw_word  (offs_t offset);					\
data32_t cpu_read##type##abits##ledw_dword (offs_t offset);					\
void     cpu_write##type##abits##ledw      (offs_t offset, data8_t data);	\
void     cpu_write##type##abits##ledw_word (offs_t offset, data16_t data);	\
void     cpu_write##type##abits##ledw_dword(offs_t offset, data32_t data);

/* ----- for declaring memory handlers ----- */
#define DECLARE_MEM_HANDLERS_8BIT(abits) \
DECLARE_HANDLERS_8BIT(mem, abits) \
void     cpu_setopbase##abits              (offs_t pc);

#define DECLARE_MEM_HANDLERS_16BIT_BE(abits) \
DECLARE_HANDLERS_16BIT_BE(mem, abits) \
void     cpu_setopbase##abits##bew         (offs_t pc);

#define DECLARE_MEM_HANDLERS_16BIT_LE(abits) \
DECLARE_HANDLERS_16BIT_LE(mem, abits) \
void     cpu_setopbase##abits##lew         (offs_t pc);

#define DECLARE_MEM_HANDLERS_32BIT_BE(abits) \
DECLARE_HANDLERS_32BIT_BE(mem, abits) \
void     cpu_setopbase##abits##bedw        (offs_t pc);

#define DECLARE_MEM_HANDLERS_32BIT_LE(abits) \
DECLARE_HANDLERS_32BIT_LE(mem, abits) \
void     cpu_setopbase##abits##ledw        (offs_t pc);

/* ----- for declaring port handlers ----- */
#define DECLARE_PORT_HANDLERS_8BIT(abits) \
DECLARE_HANDLERS_8BIT(port, abits)

#define DECLARE_PORT_HANDLERS_16BIT_BE(abits) \
DECLARE_HANDLERS_16BIT_BE(port, abits)

#define DECLARE_PORT_HANDLERS_16BIT_LE(abits) \
DECLARE_HANDLERS_16BIT_LE(port, abits)

#define DECLARE_PORT_HANDLERS_32BIT_BE(abits) \
DECLARE_HANDLERS_32BIT_BE(port, abits)

#define DECLARE_PORT_HANDLERS_32BIT_LE(abits) \
DECLARE_HANDLERS_32BIT_LE(port, abits)



/***************************************************************************

	Function prototypes for core readmem/writemem routines

***************************************************************************/

/* ----- declare 8-bit handlers ----- */
DECLARE_MEM_HANDLERS_8BIT(16)
DECLARE_MEM_HANDLERS_8BIT(17)
DECLARE_MEM_HANDLERS_8BIT(20)
DECLARE_MEM_HANDLERS_8BIT(21)
DECLARE_MEM_HANDLERS_8BIT(24)
#define change_pc16(pc)			change_pc_generic(pc, 16, 0, cpu_setopbase16)
#define change_pc17(pc) 		change_pc_generic(pc, 17, 0, cpu_setopbase17)
#define change_pc20(pc)			change_pc_generic(pc, 20, 0, cpu_setopbase20)
#define change_pc21(pc)			change_pc_generic(pc, 21, 0, cpu_setopbase21)
#define change_pc24(pc)			change_pc_generic(pc, 24, 0, cpu_setopbase24)

/* ----- declare 16-bit bigendian handlers ----- */
DECLARE_MEM_HANDLERS_16BIT_BE(16)
DECLARE_MEM_HANDLERS_16BIT_BE(18)
DECLARE_MEM_HANDLERS_16BIT_BE(24)
DECLARE_MEM_HANDLERS_16BIT_BE(32)
#define change_pc16bew(pc)		change_pc_generic(pc, 16, 1, cpu_setopbase16bew)
#define change_pc18bew(pc)      change_pc_generic(pc, 18, 1, cpu_setopbase18bew)
#define change_pc24bew(pc)		change_pc_generic(pc, 24, 1, cpu_setopbase24bew)
#define change_pc32bew(pc)		change_pc_generic(pc, 32, 1, cpu_setopbase32bew)

/* ----- declare 16-bit littleendian handlers ----- */
DECLARE_MEM_HANDLERS_16BIT_LE(16)
DECLARE_MEM_HANDLERS_16BIT_LE(17)
DECLARE_MEM_HANDLERS_16BIT_LE(24)
DECLARE_MEM_HANDLERS_16BIT_LE(29)
DECLARE_MEM_HANDLERS_16BIT_LE(32)
#define change_pc16lew(pc)		change_pc_generic(pc, 16, 1, cpu_setopbase16lew)
#define change_pc17lew(pc)		change_pc_generic(pc, 17, 1, cpu_setopbase17lew)
#define change_pc24lew(pc)		change_pc_generic(pc, 24, 1, cpu_setopbase24lew)
#define change_pc29lew(pc)		change_pc_generic(pc, 29, 1, cpu_setopbase29lew)
#define change_pc32lew(pc)		change_pc_generic(pc, 32, 1, cpu_setopbase32lew)

/* ----- declare 32-bit bigendian handlers ----- */
DECLARE_MEM_HANDLERS_32BIT_BE(24)
DECLARE_MEM_HANDLERS_32BIT_BE(29)
DECLARE_MEM_HANDLERS_32BIT_BE(32)
#define change_pc24bedw(pc)		change_pc_generic(pc, 24, 2, cpu_setopbase24bedw)
#define change_pc29bedw(pc)		change_pc_generic(pc, 29, 2, cpu_setopbase29bedw)
#define change_pc32bedw(pc)		change_pc_generic(pc, 32, 2, cpu_setopbase32bedw)

/* ----- declare 32-bit littleendian handlers ----- */
DECLARE_MEM_HANDLERS_32BIT_LE(24)
DECLARE_MEM_HANDLERS_32BIT_LE(26)
DECLARE_MEM_HANDLERS_32BIT_LE(29)
DECLARE_MEM_HANDLERS_32BIT_LE(32)
#define change_pc24ledw(pc)		change_pc_generic(pc, 24, 2, cpu_setopbase24ledw)
#define change_pc26ledw(pc)		change_pc_generic(pc, 26, 2, cpu_setopbase26ledw)
#define change_pc29ledw(pc)		change_pc_generic(pc, 29, 2, cpu_setopbase29ledw)
#define change_pc32ledw(pc)		change_pc_generic(pc, 32, 2, cpu_setopbase32ledw)

/* ----- declare pdp1 handler ----- */
DECLARE_MEM_HANDLERS_32BIT_BE(18)
#define change_pc28bedw(pc)		change_pc_generic(pc, 18, 2, cpu_setopbase18bedw)


/***************************************************************************

	Function prototypes for core readport/writeport routines

***************************************************************************/

/* ----- declare 8-bit handlers ----- */
DECLARE_PORT_HANDLERS_8BIT(16)

/* ----- declare 16-bit bigendian handlers ----- */
DECLARE_PORT_HANDLERS_16BIT_BE(16)

/* ----- declare 16-bit littleendian handlers ----- */
DECLARE_PORT_HANDLERS_16BIT_LE(16)
DECLARE_PORT_HANDLERS_16BIT_LE(24)

/* ----- declare 32-bit bigendian handlers ----- */
DECLARE_PORT_HANDLERS_32BIT_BE(16)

/* ----- declare 32-bit littleendian handlers ----- */
DECLARE_PORT_HANDLERS_32BIT_LE(16)
DECLARE_PORT_HANDLERS_32BIT_LE(24)
DECLARE_PORT_HANDLERS_32BIT_LE(32)


/***************************************************************************

	Function prototypes for core memory functions

***************************************************************************/

/* ----- memory setup function ----- */
int			memory_init(void);
void		memory_shutdown(void);
void		memory_set_context(int activecpu);
void		memory_set_unmap_value(data32_t value);

/* ----- dynamic bank handlers ----- */
void		memory_set_bankhandler_r(int bank, offs_t offset, mem_read_handler handler);
void		memory_set_bankhandler_w(int bank, offs_t offset, mem_write_handler handler);

/* ----- opcode base control ---- */
opbase_handler memory_set_opbase_handler(int cpunum, opbase_handler function);

/* ----- separate opcode/data encryption helpers ---- */
void		memory_set_opcode_base(int cpunum, void *base);
void		memory_set_encrypted_opcode_range(int cpunum, offs_t min_address,offs_t max_address);
extern offs_t encrypted_opcode_start[],encrypted_opcode_end[];

/* ----- return a base pointer to memory ---- */
void *		memory_find_base(int cpunum, offs_t offset);
void *		memory_get_read_ptr(int cpunum, offs_t offset);
void *		memory_get_write_ptr(int cpunum, offs_t offset);

/* ----- dynamic memory mapping ----- */
data8_t *	install_mem_read_handler    (int cpunum, offs_t start, offs_t end, mem_read_handler handler);
data16_t *	install_mem_read16_handler  (int cpunum, offs_t start, offs_t end, mem_read16_handler handler);
data32_t *	install_mem_read32_handler  (int cpunum, offs_t start, offs_t end, mem_read32_handler handler);
data8_t *	install_mem_write_handler   (int cpunum, offs_t start, offs_t end, mem_write_handler handler);
data16_t *	install_mem_write16_handler (int cpunum, offs_t start, offs_t end, mem_write16_handler handler);
data32_t *	install_mem_write32_handler (int cpunum, offs_t start, offs_t end, mem_write32_handler handler);

/* ----- dynamic port mapping ----- */
void		install_port_read_handler   (int cpunum, offs_t start, offs_t end, port_read_handler handler);
void		install_port_read16_handler (int cpunum, offs_t start, offs_t end, port_read16_handler handler);
void		install_port_read32_handler (int cpunum, offs_t start, offs_t end, port_read32_handler handler);
void		install_port_write_handler  (int cpunum, offs_t start, offs_t end, port_write_handler handler);
void		install_port_write16_handler(int cpunum, offs_t start, offs_t end, port_write16_handler handler);
void		install_port_write32_handler(int cpunum, offs_t start, offs_t end, port_write32_handler handler);



/***************************************************************************

	Global variables

***************************************************************************/

extern UINT8 			opcode_entry;		/* current entry for opcode fetching */
extern UINT8 *			OP_ROM;				/* opcode ROM base */
extern UINT8 *			OP_RAM;				/* opcode RAM base */
extern offs_t			OP_MEM_MIN;			/* opcode memory minimum */
extern offs_t			OP_MEM_MAX;			/* opcode memory maximum */
extern UINT8 *			cpu_bankbase[];		/* array of bank bases */
extern UINT8 *			readmem_lookup;		/* pointer to the readmem lookup table */
extern offs_t			mem_amask;			/* memory address mask */
extern struct ExtMemory	ext_memory[];		/* externally-allocated memory */

#ifdef PINMAME
/* Bank support for CODELIST */
#define FAKE_BANKID		-1
extern UINT32			cpu_bankid[];		/* array of bank ids */
#endif /* PINMAME */



/***************************************************************************

	Helper macros

***************************************************************************/

/* ----- 16/32-bit memory accessing ----- */
#define COMBINE_DATA(varptr)		(*(varptr) = (*(varptr) & mem_mask) | (data & ~mem_mask))

/* ----- 16-bit memory accessing ----- */
#define ACCESSING_LSB16				((mem_mask & 0x00ff) == 0)
#define ACCESSING_MSB16				((mem_mask & 0xff00) == 0)
#define ACCESSING_LSB				ACCESSING_LSB16
#define ACCESSING_MSB				ACCESSING_MSB16

/* ----- 32-bit memory accessing ----- */
#define ACCESSING_LSW32				((mem_mask & 0x0000ffff) == 0)
#define ACCESSING_MSW32				((mem_mask & 0xffff0000) == 0)
#define ACCESSING_LSB32				((mem_mask & 0x000000ff) == 0)
#define ACCESSING_MSB32				((mem_mask & 0xff000000) == 0)

/* ----- opcode range safety checks ----- */
#if CPUREADOP_SAFETY_NONE
#define address_is_unsafe(A)		(0)
#elif CPUREADOP_SAFETY_PARTIAL
#define address_is_unsafe(A)		(UNEXPECTED((A) > OP_MEM_MAX))
#elif CPUREADOP_SAFETY_FULL
#define address_is_unsafe(A)		((UNEXPECTED((A) < OP_MEM_MIN) || UNEXPECTED((A) > OP_MEM_MAX)))
#else
#error Must set either CPUREADOP_SAFETY_NONE, CPUREADOP_SAFETY_PARTIAL or CPUREADOP_SAFETY_FULL
#endif

/* ----- safe opcode and opcode argument reading ----- */
data8_t		cpu_readop_safe(offs_t offset);
data16_t	cpu_readop16_safe(offs_t offset);
data32_t	cpu_readop32_safe(offs_t offset);
data8_t		cpu_readop_arg_safe(offs_t offset);
data16_t	cpu_readop_arg16_safe(offs_t offset);
data32_t	cpu_readop_arg32_safe(offs_t offset);

/* ----- unsafe opcode and opcode argument reading ----- */
#define cpu_readop_unsafe(A)		(OP_ROM[(A) & mem_amask])
#define cpu_readop16_unsafe(A)		(*(data16_t *)&OP_ROM[(A) & mem_amask])
#define cpu_readop32_unsafe(A)		(*(data32_t *)&OP_ROM[(A) & mem_amask])
#define cpu_readop_arg_unsafe(A)	(OP_RAM[(A) & mem_amask])
#define cpu_readop_arg16_unsafe(A)	(*(data16_t *)&OP_RAM[(A) & mem_amask])
#define cpu_readop_arg32_unsafe(A)	(*(data32_t *)&OP_RAM[(A) & mem_amask])



#ifdef __cplusplus
}
#endif

#endif	/* !_MEMORY_H */

