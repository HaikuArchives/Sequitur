#ifndef GLPUBLIC_GLGROWTHPATTERN_H
#define GLPUBLIC_GLGROWTHPATTERN_H

#include <be/support/SupportDefs.h>

#define			GL_GROWTH_LIMIT			(256)

struct _gl_dir_pattern
{
	uint8			dir[8][GL_GROWTH_LIMIT];
	
	_gl_dir_pattern()		{ }
};

/*******************************************************
 * GL-GROWTH-PATTERN
 * This object extracts a mathematical description of
 * how to grow from a black and white texture.  It's
 * very simple -- just look at the frequency of occurrences
 * of each value next to each other.
 *******************************************************/
class GlGrowthPattern
{
public:
	GlGrowthPattern(const uint8* bytes, int32 w, int32 h);
	virtual ~GlGrowthPattern();

	status_t			InitCheck() const;
	uint8				InitialValue() const;
	uint8				NextValue(uint8 prev, uint8 dir, uint8 random) const;

protected:
	status_t			mStatus;
	_gl_dir_pattern*	mPattern[GL_GROWTH_LIMIT];
};

/*******************************************************
 * Predefined patterns
 *******************************************************/
GlGrowthPattern*		gl_new_fern_moss_pattern();
GlGrowthPattern*		gl_new_hair_cap_moss_pattern();

#endif
