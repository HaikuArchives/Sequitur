/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * A logical GUI layout engine: the programmer describes
 * high-level relationships between the different user interface
 * object through formal container classes, which then take care
 * of their physical placement.  The system is completely
 * font-sensitive and resizeable.
 *
 * ----------------------------------------------------------------------
 *
 * ArpBaseLayout.h
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * • Integrate ArpViewWrapper with the ArpBaseLayout base
 *   object?
 * • Improve name handling: always change an associated view
 *   when the layoutable name is changed, return the associated
 *   view's name instead of the layout object's if possible.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * Dec 6, 1998:
 *	First public release.
 *
 * May 5, 1999:
 *	- Added "ArpAnySize" constant.
 *
 */

/**
@author Dianne Hackborn
@package ArpLayout
**/
 
#ifndef ARPLAYOUT_ARPDIMENS_H
#define ARPLAYOUT_ARPDIMENS_H

#ifndef _MESSAGE_H
#include <be/app/Message.h>
#endif

/**
Place this value into the maximum width/height to allow
unbounded expansion.
@defvar ArpAnySize const float ArpAnySize
*/
extern const float ArpAnySize;

/**
Return true if the given floating point value indicates
that there is no maximum limit.
@deffunc bool IsAnySize(float val)
*/
bool IsAnySize(float val);

/** -----------------------------------------------------------------

This is information about a single dimension (X or Y).

    ----------------------------------------------------------------- */

class ArpUniDimens
{
public:
	ArpUniDimens();
	ArpUniDimens(float pre, float minb, float prefb, float maxb, float post);
	ArpUniDimens(const ArpUniDimens& o);
	~ArpUniDimens();
	
	ArpUniDimens& operator=(const ArpUniDimens& o);
	
	void Init();
	void SetTo(float pre, float minb, float prefb, float maxb, float post);
	
	void SetPreLabel(float value);
	float PreLabel() const;
	
	void SetMinBody(float value);
	float MinBody() const;
	
	void SetPrefBody(float value);
	float PrefBody() const;
	
	void SetMaxBody(float value);
	float MaxBody() const;
	
	void SetPostLabel(float value);
	float PostLabel() const;
	
	void SetBody(float minb, float prefb, float maxb);
	
	float TotalMin() const;
	float TotalPref() const;
	float TotalMax() const;
	
	void AddBody(float value);
	void AddBody(const ArpUniDimens& dim);
	
	void AddLabel(float pre, float post);
	
	void MakeAllBody();
	
	void SetMinTotal(float value);
	
	void SetMinDimens(const ArpUniDimens& dim);
	void SetMaxDimens(const ArpUniDimens& dim);
	
private:
	bool validate() const;
	
	float mPreLabel;
	float mMinBody;
	float mPrefBody;
	float mMaxBody;
	float mPostLabel;
};

/** -----------------------------------------------------------------

This class is used to pass information about the basic
display dimensions of a user interface element.  This
is an extension to the existing GetPreferredSize() function
call, providing more detailed information about how
an object can be dimensioned.

    ----------------------------------------------------------------- */

class ArpDimens
{
public:
	ArpDimens();
	ArpDimens(const ArpUniDimens& x, const ArpUniDimens& y);
	ArpDimens(const ArpDimens& o);
	~ArpDimens();
	
	ArpDimens& operator=(const ArpDimens& o);
	
	void Init();
	void SetTo(const ArpUniDimens& x, const ArpUniDimens& y);

	void SetX(const ArpUniDimens& dimens);
	const ArpUniDimens& X() const;
	ArpUniDimens& X();
	
	void SetY(const ArpUniDimens& dimens);
	const ArpUniDimens& Y() const;
	ArpUniDimens& Y();
	
private:
	ArpUniDimens mX;
	ArpUniDimens mY;
};

#endif
