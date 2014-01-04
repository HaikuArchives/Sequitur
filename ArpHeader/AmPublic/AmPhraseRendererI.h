/* AmPhraseRendererI.h
 * Copyright (c)2001 by Eric Hackborn.
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
 * 2001.03.06		hackborn@angryredplanet.com
 * Abstracted this file from the old AmMidiPhraseRenderer.
 */

#ifndef AMPUBLIC_AMPHRASERENDERERI_H
#define AMPUBLIC_AMPHRASERENDERERI_H

#include <interface/View.h>
#include "ArpKernel/ArpLineArrayCache.h"
class AmPhraseEvent;
class AmTrack;

/*************************************************************************
 * AM-PHRASE-RENDERER-I
 *************************************************************************/
class AmPhraseRendererI
{	
public:
	virtual	~AmPhraseRendererI()	{ }
	
	virtual void	BeginTrack(	BRect clip,
								BView* view,
								const AmTrack* track,
								ArpLineArrayCache& lineCache) = 0;
	virtual void	DrawPhrase(	BRect clip,
								BView* view,
								const AmTrack* track,
								const AmPhraseEvent* phrase,
								AmTime end,
								AmPhraseEvent* topPhrase,
								ArpLineArrayCache& lineCache) = 0;
	virtual void	EndTrack(	BRect clip,
								BView* view,
								const AmTrack* track,
								ArpLineArrayCache& lineCache) = 0;

	virtual AmPhraseRendererI*	Copy() const = 0;
};


#endif 
