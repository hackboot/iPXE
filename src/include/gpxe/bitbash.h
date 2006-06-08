#ifndef _GPXE_BITBASH_H
#define _GPXE_BITBASH_H

/** @file
 *
 * Bit-bashing interfaces
 *
 */

/** A bit-bashing interface */
struct bit_basher {
	/**
	 * Set/clear output bit
	 *
	 * @v basher		Bit-bashing interface
	 * @v bit_id		Bit number
	 * @v data		Value to write
	 * 
	 * @c data will be 0 if a logic 0 should be written (i.e. the
	 * bit should be cleared), or -1UL if a logic 1 should be
	 * written (i.e. the bit should be set).  This is done so that
	 * the method may simply binary-AND @c data with the
	 * appropriate bit mask.
	 */
	void ( * write ) ( struct bit_basher *basher, unsigned int bit_id,
			   unsigned long data );
	/**
	 * Read input bit
	 *
	 * @v basher		Bit-bashing interface
	 * @v bit_id		Bit number
	 * @ret zero		Input is a logic 0
	 * @ret non-zero	Input is a logic 1
	 */
	int ( * read ) ( struct bit_basher *basher, unsigned int bit_id );
	/** Delay between subsequent calls to write(), in microseconds */
	unsigned int udelay;
};

extern void write_bit ( struct bit_basher *basher, unsigned int bit_id,
			unsigned long data );
extern int read_bit ( struct bit_basher *basher, unsigned int bit_id );

#endif /* _GPXE_BITBASH_H */
