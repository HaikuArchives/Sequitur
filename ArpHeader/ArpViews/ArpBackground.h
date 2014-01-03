/* ArpBackground.h */

#ifndef ARPVIEWS_ARPBACKGROUND_H
#define ARPVIEWS_ARPBACKGROUND_H

#include <interface/View.h>
#include <support/SupportDefs.h>

/*************************************************************************
 * ARP-BACKGROUND
 * This is the abstract superclass for all objects that paint backgrounds
 * onto views.  Backgrounds are a simple singly-linked list to allow a view
 * to have any number of backgrounds.  All subclasses are responsible for
 * implementing the DrawOn() method.  When the destructor is called, all
 * next backgrounds get deleted, as well.
 *************************************************************************/
class ArpBackground
{
public:
	ArpBackground();
	virtual ~ArpBackground();
	
	/* Add a new background to the end of the list I'm in.
	 */
	status_t AddTail(ArpBackground* tail);

	/* Draw this background and all next backgrounds on the
	 * supplied view.
	 */
	void DrawAllOn(BView* view, BRect clip);

protected:
	ArpBackground	*next;

	/* Subclasses must implement to perform their drawing on the
	 * supplied view, in the supplied clipping rect.
	 */
	virtual void DrawOn(BView* view, BRect clip) = 0;
};

/*************************************************************************
 * ARP-CENTER-BACKGROUND
 * Draw a line in the center of the view.  The line colour and view that
 * has the bounds I find the center of must be supplied in the constructor.
 *************************************************************************/
class ArpCenterBackground : public ArpBackground
{
public:
	ArpCenterBackground(BView* boundsView, rgb_color lineColor);
	
protected:
	virtual void DrawOn(BView* view, BRect clip);

private:
	BView*		mBoundsView;
	rgb_color	mLineColor;
};

/*************************************************************************
 * ARP-FLOOR-BACKGROUND
 * Draw a line at the bottom of the view.  The line colour and view that
 * has the bounds I find the bottom of must be supplied in the constructor.
 *************************************************************************/
class ArpFloorBackground : public ArpBackground
{
public:
	ArpFloorBackground(BView* boundsView, rgb_color lineColor);
		
protected:
	virtual void DrawOn(BView* view, BRect clip);

private:
	BView*		mBoundsView;
	rgb_color	mLineColor;
};

#endif
