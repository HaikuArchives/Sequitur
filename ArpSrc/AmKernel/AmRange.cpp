/* AmRange.cpp
*/
#define _BUILDING_AmKernel 1

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "AmPublic/AmRange.h"

/***************************************************************************
 * AM-RANGE
 ****************************************************************************/
AmRange::AmRange()
		: start(-1), end(-1)
{
}

AmRange::AmRange(AmTime S, AmTime E)
		: start(S), end(E)
{
}

AmRange::AmRange(const AmRange& o)
		: start(o.start), end(o.end)
{
}

bool AmRange::operator==(const AmRange& r) const
{
	return (start == r.start) && (end == r.end);
}

bool AmRange::operator!=(const AmRange& r) const
{
	return (start != r.start) || (end != r.end);
}

AmRange& AmRange::operator=(const AmRange &r)
{
	start = r.start;
	end = r.end;
	return *this;
}

AmRange AmRange::operator+(const AmRange& r) const
{
	AmTime	newStart, newEnd;
	if( start == -1 ) newStart = r.start;
	else if( r.start == -1 ) newStart = start;
	else newStart = (start < r.start) ? start : r.start;

	if( end == -1 ) newEnd = r.end;
	else if( r.end == -1 ) newEnd = end;
	else newEnd = (end > r.end) ? end : r.end;
	
	return AmRange( newStart, newEnd );
}

AmRange AmRange::operator-(const AmRange& r) const
{
	AmTime	newStart, newEnd;
	if( start == -1 ) newStart = r.start;
	else if( r.start == -1 ) newStart = start;
	else newStart = (start > r.start) ? start : r.start;

	if( end == -1 ) newEnd = r.end;
	else if( r.end == -1 ) newEnd = end;
	else newEnd = (end < r.end) ? end : r.end;
	
	return AmRange( newStart, newEnd );
}

AmRange& AmRange::operator+=(const AmRange& r)
{
	if (start == -1) {
		start = r.start;
	} else if (r.start != -1) {
		if (r.start < start) start = r.start;
	}
	if (end == -1) {
		end = r.end;
	} else if (r.end != -1) {
		if (r.end > end) end = r.end;
	}
	return *this;
}

AmRange& AmRange::operator-=(const AmRange& r)
{
	if (start == -1) {
		start = r.start;
	} else if (r.start != -1) {
		if (r.start > start) start = r.start;
	}
	if (end == -1) {
		end = r.end;
	} else if (r.end != -1) {
		if (r.end < end) end = r.end;
	}
	return *this;
}

void AmRange::Set(AmTime S, AmTime E)
{
	start = S;
	end = E;
}

bool AmRange::IsValid() const
{
	return (start >= 0) && (end >= 0);
}

void AmRange::MakeInvalid()
{
	start = end = -1;
}

bool AmRange::Overlaps(AmRange range) const
{
	if (end < range.start) return false;
	if (start > range.end) return false;
	return true;
}

bool AmRange::Contains(AmTime time) const
{
	if (start < 0 || end < 0) return false;
	return time >= start && time <= end;
}

bool AmRange::Contains(AmRange range) const
{
	if (!IsValid() || !range.IsValid()) return false;
	return start <= range.start && end >= range.end;
}

void AmRange::Print() const
{
	printf("AmRange from %lld to %lld\n", start, end);
}
