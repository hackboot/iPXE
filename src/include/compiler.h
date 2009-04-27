#ifndef COMPILER_H
#define COMPILER_H

/*
 * Doxygen can't cope with some of the more esoteric areas of C, so we
 * make its life simpler.
 *
 */
#ifdef DOXYGEN
#define __attribute__(x)
#endif

/** @file
 *
 * Global compiler definitions.
 *
 * This file is implicitly included by every @c .c file in Etherboot.
 * It defines global macros such as DBG().
 *
 * We arrange for each object to export the symbol @c obj_OBJECT
 * (where @c OBJECT is the object name, e.g. @c rtl8139) as a global
 * symbol, so that the linker can drag in selected object files from
 * the library using <tt> -u obj_OBJECT </tt>.
 *
 */

/* Force visibility of all symbols to "hidden", i.e. inform gcc that
 * all symbol references resolve strictly within our final binary.
 * This avoids unnecessary PLT/GOT entries on x86_64.
 *
 * This is a stronger claim than specifying "-fvisibility=hidden",
 * since it also affects symbols marked with "extern".
 */
#ifndef ASSEMBLY
#if __GNUC__ >= 4
#pragma GCC visibility push(hidden)
#endif
#endif /* ASSEMBLY */

/**
 * @defgroup symmacros Macros to provide or require explicit symbols
 * @{
 */

/** Provide a symbol within this object file */
#ifdef ASSEMBLY
#define PROVIDE_SYMBOL( _sym )				\
	.globl	_sym ;					\
	.comm	_sym, 0
#else /* ASSEMBLY */
#define PROVIDE_SYMBOL( _sym )				\
	char _sym[0]
#endif /* ASSEMBLY */

/** Require a symbol within this object file */
#ifdef ASSEMBLY
#define REQUIRE_SYMBOL( _sym )				\
	.equ	__need_ # _sym, _sym
#else /* ASSEMBLY */
#define REQUIRE_SYMBOL( _sym )				\
	__asm__ ( ".equ\t__need_" #_sym ", " #_sym )
#endif /* ASSEMBLY */

/** @} */

/**
 * @defgroup objmacros Macros to provide or require explicit objects
 * @{
 */

/* Not quite sure why cpp requires two levels of macro call in order
 * to actually expand OBJECT...
 */
#undef _H1
#define _H1( x, y ) x ## y
#undef _H2
#define _H2( x, y ) _H1 ( x, y )
#define PREFIX_OBJECT( _prefix ) _H2 ( _prefix, OBJECT )
#define OBJECT_SYMBOL PREFIX_OBJECT ( obj_ )

/** Always provide the symbol for the current object (defined by -DOBJECT) */
PROVIDE_SYMBOL ( OBJECT_SYMBOL );

/** Explicitly require another object */
#define REQUIRE_OBJECT( _obj ) REQUIRE_SYMBOL ( obj_ ## _obj )

/** @} */

/** Select file identifier for errno.h (if used) */
#define ERRFILE PREFIX_OBJECT ( ERRFILE_ )

/** @defgroup dbg Debugging infrastructure
 * @{
 */
#ifndef ASSEMBLY

/** @def DBG
 *
 * Print a debugging message.
 *
 * The debug level is set at build time by specifying the @c DEBUG=
 * parameter on the @c make command line.  For example, to enable
 * debugging for the PCI bus functions (in pci.c) in a @c .dsk image
 * for the @c rtl8139 card, you could use the command line
 *
 * @code
 *
 *   make bin/rtl8139.dsk DEBUG=pci
 *
 * @endcode
 *
 * This will enable the debugging statements (DBG()) in pci.c.  If
 * debugging is not enabled, DBG() statements will be ignored.
 *
 * You can enable debugging in several objects simultaneously by
 * separating them with commas, as in
 *
 * @code
 *
 *   make bin/rtl8139.dsk DEBUG=pci,buffer,heap
 *
 * @endcode
 *
 * You can increase the debugging level for an object by specifying it
 * with @c :N, where @c N is the level, as in
 *
 * @code
 *
 *   make bin/rtl8139.dsk DEBUG=pci,buffer:2,heap
 *
 * @endcode
 *
 * which would enable debugging for the PCI, buffer-handling and
 * heap-allocation code, with the buffer-handling code at level 2.
 *
 */

/*
 * If debug_OBJECT is set to a true value, the macro DBG(...) will
 * expand to printf(...) when compiling OBJECT, and the symbol
 * DEBUG_LEVEL will be inserted into the object file.
 *
 */
#define DEBUG_SYMBOL PREFIX_OBJECT ( debug_ )

/** printf() for debugging
 *
 * This function exists so that the DBG() macros can expand to
 * printf() calls without dragging the printf() prototype into scope.
 *
 * As far as the compiler is concerned, dbg_printf() and printf() are
 * completely unrelated calls; it's only at the assembly stage that
 * references to the dbg_printf symbol are collapsed into references
 * to the printf symbol.
 */
extern int __attribute__ (( format ( printf, 1, 2 ) )) 
dbg_printf ( const char *fmt, ... ) asm ( "printf" );

extern void dbg_autocolourise ( unsigned long id );
extern void dbg_decolourise ( void );
extern void dbg_hex_dump_da ( unsigned long dispaddr,
			      const void *data, unsigned long len );

#if DEBUG_SYMBOL
#define DBGLVL_MAX DEBUG_SYMBOL
#else
#define DBGLVL_MAX 0
#endif

/* Allow for selective disabling of enabled debug levels */
#if DBGLVL_MAX
int __debug_disable;
#define DBGLVL ( DBGLVL_MAX & ~__debug_disable )
#define DBG_DISABLE( level ) do {				\
	__debug_disable |= ( (level) & DBGLVL_MAX );		\
	} while ( 0 )
#define DBG_ENABLE( level ) do {				\
	__debug_disable &= ~( (level) & DBGLVL_MAX );		\
	} while ( 0 )
#else
#define DBGLVL 0
#define DBG_DISABLE( level ) do { } while ( 0 )
#define DBG_ENABLE( level ) do { } while ( 0 )
#endif

#define DBGLVL_LOG	1
#define DBG_LOG		( DBGLVL & DBGLVL_LOG )
#define DBGLVL_EXTRA	2
#define DBG_EXTRA	( DBGLVL & DBGLVL_EXTRA )
#define DBGLVL_PROFILE	4
#define DBG_PROFILE	( DBGLVL & DBGLVL_PROFILE )
#define DBGLVL_IO	8
#define DBG_IO		( DBGLVL & DBGLVL_IO )

/**
 * Print debugging message if we are at a certain debug level
 *
 * @v level		Debug level
 * @v ...		printf() argument list
 */
#define DBG_IF( level, ... ) do {				\
		if ( DBG_ ## level ) {				\
			dbg_printf ( __VA_ARGS__ );		\
		}						\
	} while ( 0 )

/**
 * Print a hex dump if we are at a certain debug level
 *
 * @v level		Debug level
 * @v dispaddr		Display address
 * @v data		Data to print
 * @v len		Length of data
 */
#define DBG_HDA_IF( level, dispaddr, data, len )  do {		\
		if ( DBG_ ## level ) {				\
			union {					\
				unsigned long ul;		\
				typeof ( dispaddr ) raw;	\
			} da;					\
			da.raw = dispaddr;			\
			dbg_hex_dump_da ( da.ul, data, len );	\
		}						\
	} while ( 0 )

/**
 * Print a hex dump if we are at a certain debug level
 *
 * @v level		Debug level
 * @v data		Data to print
 * @v len		Length of data
 */
#define DBG_HD_IF( level, data, len ) do {			\
		const void *_data = data;			\
		DBG_HDA_IF ( level, _data, _data, len );	\
	} while ( 0 )

/**
 * Select colour for debug messages if we are at a certain debug level
 *
 * @v level		Debug level
 * @v id		Message stream ID
 */
#define DBG_AC_IF( level, id ) do {				\
		if ( DBG_ ## level ) {				\
			union {					\
				unsigned long ul;		\
				typeof ( id ) raw;		\
			} dbg_stream;				\
			dbg_stream.raw = id;			\
			dbg_autocolourise ( dbg_stream.ul );	\
		}						\
	} while ( 0 )

/**
 * Revert colour for debug messages if we are at a certain debug level
 *
 * @v level		Debug level
 */
#define DBG_DC_IF( level ) do {					\
		if ( DBG_ ## level ) {				\
			dbg_decolourise();			\
		}						\
	} while ( 0 )

/* Autocolourising versions of the DBGxxx_IF() macros */

#define DBGC_IF( level, id, ... ) do {				\
		DBG_AC_IF ( level, id );			\
		DBG_IF ( level, __VA_ARGS__ );			\
		DBG_DC_IF ( level );				\
	} while ( 0 )

#define DBGC_HDA_IF( level, id, ... ) do {			\
		DBG_AC_IF ( level, id );			\
		DBG_HDA_IF ( level, __VA_ARGS__ );		\
		DBG_DC_IF ( level );				\
	} while ( 0 )

#define DBGC_HD_IF( level, id, ... ) do {			\
		DBG_AC_IF ( level, id );			\
		DBG_HD_IF ( level, __VA_ARGS__ );		\
		DBG_DC_IF ( level );				\
	} while ( 0 )

/* Versions of the DBGxxx_IF() macros that imply DBGxxx_IF( LOG, ... )*/

#define DBG( ... )		DBG_IF		( LOG, __VA_ARGS__ )
#define DBG_HDA( ... )		DBG_HDA_IF	( LOG, __VA_ARGS__ )
#define DBG_HD( ... )		DBG_HD_IF	( LOG, __VA_ARGS__ )
#define DBGC( ... )		DBGC_IF		( LOG, __VA_ARGS__ )
#define DBGC_HDA( ... )		DBGC_HDA_IF	( LOG, __VA_ARGS__ )
#define DBGC_HD( ... )		DBGC_HD_IF	( LOG, __VA_ARGS__ )

/* Versions of the DBGxxx_IF() macros that imply DBGxxx_IF( EXTRA, ... )*/

#define DBG2( ... )		DBG_IF		( EXTRA, __VA_ARGS__ )
#define DBG2_HDA( ... )		DBG_HDA_IF	( EXTRA, __VA_ARGS__ )
#define DBG2_HD( ... )		DBG_HD_IF	( EXTRA, __VA_ARGS__ )
#define DBGC2( ... )		DBGC_IF		( EXTRA, __VA_ARGS__ )
#define DBGC2_HDA( ... )	DBGC_HDA_IF	( EXTRA, __VA_ARGS__ )
#define DBGC2_HD( ... )		DBGC_HD_IF	( EXTRA, __VA_ARGS__ )

/* Versions of the DBGxxx_IF() macros that imply DBGxxx_IF( PROFILE, ... )*/

#define DBGP( ... )		DBG_IF		( PROFILE, __VA_ARGS__ )
#define DBGP_HDA( ... )		DBG_HDA_IF	( PROFILE, __VA_ARGS__ )
#define DBGP_HD( ... )		DBG_HD_IF	( PROFILE, __VA_ARGS__ )
#define DBGCP( ... )		DBGC_IF		( PROFILE, __VA_ARGS__ )
#define DBGCP_HDA( ... )	DBGC_HDA_IF	( PROFILE, __VA_ARGS__ )
#define DBGCP_HD( ... )		DBGC_HD_IF	( PROFILE, __VA_ARGS__ )

/* Versions of the DBGxxx_IF() macros that imply DBGxxx_IF( IO, ... )*/

#define DBGIO( ... )		DBG_IF		( IO, __VA_ARGS__ )
#define DBGIO_HDA( ... )	DBG_HDA_IF	( IO, __VA_ARGS__ )
#define DBGIO_HD( ... )		DBG_HD_IF	( IO, __VA_ARGS__ )
#define DBGCIO( ... )		DBGC_IF		( IO, __VA_ARGS__ )
#define DBGCIO_HDA( ... )	DBGC_HDA_IF	( IO, __VA_ARGS__ )
#define DBGCIO_HD( ... )	DBGC_HD_IF	( IO, __VA_ARGS__ )


#if DEBUG_SYMBOL == 0
#define NDEBUG
#endif

#endif /* ASSEMBLY */
/** @} */

/** @defgroup attrs Miscellaneous attributes
 * @{
 */
#ifndef ASSEMBLY

/** Declare a data structure as packed. */
#define PACKED __attribute__ (( packed ))

/** Declare a variable or data structure as unused. */
#define __unused __attribute__ (( unused ))

/**
 * Declare a function as pure - i.e. without side effects
 */
#define __pure __attribute__ (( pure ))

/**
 * Declare a function as const - i.e. it does not access global memory
 * (including dereferencing pointers passed to it) at all.
 * Must also not call any non-const functions.
 */
#define __const __attribute__ (( const ))

/**
 * Declare a function's pointer parameters as non-null - i.e. force
 * compiler to check pointers at compile time and enable possible
 * optimizations based on that fact
 */
#define __nonnull __attribute__ (( nonnull ))

/**
 * Declare a pointer returned by a function as a unique memory address
 * as returned by malloc-type functions.
 */
#define __malloc __attribute__ (( malloc ))

/**
 * Declare a function as used.
 *
 * Necessary only if the function is called only from assembler code.
 */
#define __used __attribute__ (( used ))

/** Declare a data structure to be aligned with 16-byte alignment */
#define __aligned __attribute__ (( aligned ( 16 ) ))

/** Declare a function to be always inline */
#define __always_inline __attribute__ (( always_inline ))

/**
 * Shared data.
 *
 * To save space in the binary when multiple-driver images are
 * compiled, uninitialised data areas can be shared between drivers.
 * This will typically be used to share statically-allocated receive
 * and transmit buffers between drivers.
 *
 * Use as e.g.
 *
 * @code
 *
 *   struct {
 *	char	rx_buf[NUM_RX_BUF][RX_BUF_SIZE];
 *	char	tx_buf[TX_BUF_SIZE];
 *   } my_static_data __shared;
 *
 * @endcode
 *
 */
#define __shared __asm__ ( "_shared_bss" ) __aligned

#endif /* ASSEMBLY */
/** @} */

/**
 * Optimisation barrier
 */
#ifndef ASSEMBLY
#define barrier() __asm__ __volatile__ ( "" : : : "memory" )
#endif /* ASSEMBLY */

/**
 * @defgroup licences Licence declarations
 *
 * For reasons that are partly historical, various different files
 * within the gPXE codebase have differing licences.
 *
 * @{
 */

/** Declare a file as being in the public domain
 *
 * This licence declaration is applicable when a file states itself to
 * be in the public domain.
 */
#define FILE_LICENCE_PUBLIC_DOMAIN \
	PROVIDE_SYMBOL ( __licence_public_domain )

/** Declare a file as being under version 2 (or later) of the GNU GPL
 *
 * This licence declaration is applicable when a file states itself to
 * be licensed under the GNU GPL; "either version 2 of the License, or
 * (at your option) any later version".
 */
#define FILE_LICENCE_GPL2_OR_LATER \
	PROVIDE_SYMBOL ( __licence_gpl2_or_later )

/** Declare a file as being under version 2 of the GNU GPL
 *
 * This licence declaration is applicable when a file states itself to
 * be licensed under version 2 of the GPL, and does not include the
 * "or, at your option, any later version" clause.
 */
#define FILE_LICENCE_GPL2_ONLY \
	PROVIDE_SYMBOL ( __licence_gpl2_only )

/** Declare a file as being under any version of the GNU GPL
 *
 * This licence declaration is applicable when a file states itself to
 * be licensed under the GPL, but does not specify a version.
 *
 * According to section 9 of the GPLv2, "If the Program does not
 * specify a version number of this License, you may choose any
 * version ever published by the Free Software Foundation".
 */
#define FILE_LICENCE_GPL_ANY \
	PROVIDE_SYMBOL ( __licence_gpl_any )

/** Declare a file as being under the three-clause BSD licence
 *
 * This licence declaration is applicable when a file states itself to
 * be licensed under terms allowing redistribution in source and
 * binary forms (with or without modification) provided that:
 *
 *     redistributions of source code retain the copyright notice,
 *     list of conditions and any attached disclaimers
 *
 *     redistributions in binary form reproduce the copyright notice,
 *     list of conditions and any attached disclaimers in the
 *     documentation and/or other materials provided with the
 *     distribution
 *
 *     the name of the author is not used to endorse or promote
 *     products derived from the software without specific prior
 *     written permission
 *
 * It is not necessary for the file to explicitly state that it is
 * under a "BSD" licence; only that the licensing terms be
 * functionally equivalent to the standard three-clause BSD licence.
 */
#define FILE_LICENCE_BSD3 \
	PROVIDE_SYMBOL ( __licence_bsd3 )

/** Declare a file as being under the two-clause BSD licence
 *
 * This licence declaration is applicable when a file states itself to
 * be licensed under terms allowing redistribution in source and
 * binary forms (with or without modification) provided that:
 *
 *     redistributions of source code retain the copyright notice,
 *     list of conditions and any attached disclaimers
 *
 *     redistributions in binary form reproduce the copyright notice,
 *     list of conditions and any attached disclaimers in the
 *     documentation and/or other materials provided with the
 *     distribution
 *
 * It is not necessary for the file to explicitly state that it is
 * under a "BSD" licence; only that the licensing terms be
 * functionally equivalent to the standard two-clause BSD licence.
 */
#define FILE_LICENCE_BSD2 \
	PROVIDE_SYMBOL ( __licence_bsd2 )

/** Declare a particular licence as applying to a file */
#define FILE_LICENCE( _licence ) FILE_LICENCE_ ## _licence

/** @} */

/* This file itself is under GPLv2-or-later */
FILE_LICENCE ( GPL2_OR_LATER );

#include <bits/compiler.h>

#endif /* COMPILER_H */
