/* AmGraphicEffect.h
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
 * 02.19.01		hackborn@angryredplanet.com
 * Created this file
 */
#ifndef AMPUBLIC_AMGRAPHICEFFECT_H
#define AMPUBLIC_AMGRAPHICEFFECT_H

#include <be/interface/View.h>
#include "AmPublic/AmDefs.h"

/***************************************************************************
 * AM-GRAPHIC-EFFECT
 * This is the abstract interface for classes that will draw visuals on
 * other views.  It is used as optional polish for some of the AmTools.
 ***************************************************************************/
class AmGraphicEffect
{
public:
	AmGraphicEffect(tool_id ownerId);
	virtual ~AmGraphicEffect();

	/* From 0 to the number of effects, fill in name with a user-friendly
	 * label and key with the unique effect name, and answer
	 * with B_OK.  Answer with B_ERROR if index is too high.
	 */
	static status_t				GetEffectInfo(	uint32 index,
												BString& outLabel,
												BString& outKey);
	static status_t				GetEffectInfo(	const BString& key,
												BString& outLabel);
	static AmGraphicEffect*		NewEffect(const BString& key, tool_id id);

	/* Answer the tool that created this effect.
	 */
	tool_id						OwnerId();

	/* Answer true if this effect is over.
	 */
	virtual bool				IsFinished() const = 0;
	/* Initialization hook.
	 */
	virtual void				Begin(BView* target, BPoint pt) = 0;
	virtual void				MouseMoved(BPoint where, uint32 code) = 0;
	virtual void				DrawOn(BView* view, BRect clip) = 0;

private:
	virtual	void				_ReservedAmGraphicEffect1();
	virtual	void				_ReservedAmGraphicEffect2();
	virtual	void				_ReservedAmGraphicEffect3();
	virtual	void				_ReservedAmGraphicEffect4();
	virtual	void				_ReservedAmGraphicEffect5();
	virtual	void				_ReservedAmGraphicEffect6();
	virtual	void				_ReservedAmGraphicEffect7();
	virtual	void				_ReservedAmGraphicEffect8();
	virtual	void				_ReservedAmGraphicEffect9();
	virtual	void				_ReservedAmGraphicEffect10();
	virtual	void				_ReservedAmGraphicEffect11();
	virtual	void				_ReservedAmGraphicEffect12();
	virtual	void				_ReservedAmGraphicEffect13();
	virtual	void				_ReservedAmGraphicEffect14();
	virtual	void				_ReservedAmGraphicEffect15();
	virtual	void				_ReservedAmGraphicEffect16();
	size_t						_reserved_data[12];
	
	tool_id		mOwnerId;
};

#endif 
