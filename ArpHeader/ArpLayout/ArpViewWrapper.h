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
 * ArpViewWrapper.h
 *
 * An ArpLayoutable that can be used to wrap around another
 * BView, without requiring multiple inheritance or subclassing.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * * I have been unable to get many of the Be-supplied controls
 *   to respond with meaningful GetPreferredSize() files.  This
 *   means they require subclassing to work with the ArpLayoutable
 *   system.  See ViewStubs.h for subclasses that have been implemented.
 *
 *   So far, the views I know that DON'T need subclassing are
 *   BColorControl and BCheckBox.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * Dec 6, 1998:
 *	First public release.
 *
 */

#ifndef ARPLAYOUT_ARPVIEWWRAPPER_H
#define ARPLAYOUT_ARPVIEWWRAPPER_H

#ifndef ARPLAYOUT_ARPLAYOUT_H
#include <ArpLayout/ArpLayout.h>
#endif

#ifndef ARPLAYOUT_ARPLAYOUTHOOKS_H
#include <ArpLayout/ArpLayoutHooks.h>
#endif

#ifndef _VIEW_H
#include <be/interface/View.h>
#endif

class _EXPORT ArpViewWrapper : public ArpLayout {
private:
  	typedef	ArpLayout inherited;
  	
public:
  	ArpViewWrapper(BView* wrapview);
  	ArpViewWrapper(BMessage* data, bool final=true);
  	virtual ~ArpViewWrapper();
  	
	static ArpViewWrapper*	Instantiate(BMessage* archive);
	virtual status_t		Archive(BMessage* data, bool deep=true) const;

	/* Parameters:
	 * "HorridSizeKludge" (bool)  If set, use a nasty kludge to
	 * try to extract the preferred size from standard Be controls.
	 */
	static const char* HorridSizeKludgeP;
	
	virtual void MessageReceived(BMessage *message);
	virtual BHandler*
	ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
					 int32 form, const char *property);
	virtual status_t GetSupportedSuites(BMessage *data);
	virtual BHandler* LayoutHandler();
	virtual const BHandler* LayoutHandler() const;
	BView*	OwnerView(void);
	
protected:
	virtual void	ComputeDimens(ArpDimens& dimens);
	
private:
	ArpParam<bool>	PV_HorridSizeKludge;
	
	void			initialize();
	
	BView*	view;
};

#endif
