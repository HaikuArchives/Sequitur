/* ArpControlLayer.h
 * Copyright (c)2002 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
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
 * 2002.08.01			hackborn@angryredplanet.com
 * Created this file.
 */
 
#ifndef ARPINTERFACE_ARPCONTROLLAYER_H
#define ARPINTERFACE_ARPCONTROLLAYER_H

#include <be/support/TypeConstants.h>
#include <ArpCore/StlVector.h>
class _ControlLayerEntry;

/*******************************************************
 * ARP-CONTROL-LAYER
 * A control layer is a panel of controls that popup when
 * the user is in a certain area of the view.
 *******************************************************/
class ArpControlLayer
{
public:
	ArpControlLayer();
	virtual ~ArpControlLayer();

	void				SetEnabled(bool on);
	bool				IsEnabled() const;
	/* NOTE, this class neither owns nor copies the supplied
	 * bitmaps, supplier must make sure they exist for the
	 * life of the control layer.  Note that the id must be
	 * 0 or greater.
	 */
	status_t			AddControl(	int32 id, const ArpBitmap* exposed,
									const ArpBitmap* over);
	status_t			GetControl(	int32 id, bool* outEnabled, BRect* outFrame) const;
	/* No control will be returned if there's one at the
	 * point but it's disabled.
	 */
	status_t			ControlAt(	const BPoint& pt, int32* outCtrlId,
									BRect* outFrame = 0) const;

	void				DrawOn(BRect clip, BView* v);

	/* Answer B_OK if the mouse was pressed on an active control,
	 * and supply the control's id.  The MouseUp() only answers
	 * B_OK if the control id is the same as the MouseDown()'s.
	 */
	status_t			MouseDown(BPoint where, int32* outId = 0);
	status_t			MouseUp(BPoint where, int32* outId = 0);
	
protected:
	/* Set the frame for the control at the supplied id.
	 */
	status_t			SetControlProps(int32 id, BPoint origin, bool enabled);

private:
	vector<_ControlLayerEntry*>	mEntries;
	enum {
		ENABLED_F		= 0x00000001
	};
	uint32				mFlags;
	int32				mMouseDownId;
	
	bool				HasEntry(int32 id) const;
};


#endif
