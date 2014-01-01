/* AmRange.h
 * Copyright (c)2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 09.07.98		hackborn
 * Created this file
 */

#ifndef AMKERNEL_AMRANGE_H
#define AMKERNEL_AMRANGE_H

#include "AmPublic/AmDefs.h"

/***************************************************************************
 * AM-RANGE
 * This class represents a range of time in a sequence.
 ****************************************************************************/
class AmRange
{
public:
	AmRange();
	AmRange(AmTime start, AmTime end);
	AmRange(const AmRange& o);

	AmTime		start, end;

	bool		operator==(const AmRange& r) const;
	bool		operator!=(const AmRange& r) const;
	AmRange&	operator=(const AmRange &r);
	AmRange		operator+(const AmRange& r) const;
	AmRange		operator-(const AmRange& r) const;
	AmRange&	operator+=(const AmRange& r);
	AmRange&	operator-=(const AmRange& r);
	void		Set(AmTime start, AmTime end);

	/* AmRanges that haven't been set yet are considered invalid.
	 * This is currently defined as both start and end being less than 0.
	 */
	bool		IsValid() const;
	/* Make this object respond false to IsValid().
	 */
	void		MakeInvalid();
	/* Answer true if this range overlaps the supplied range.
	 */
	bool		Overlaps(AmRange range) const;
	bool		Contains(AmTime time) const;
	bool		Contains(AmRange range) const;
	
	void		Print() const;
};

#endif
