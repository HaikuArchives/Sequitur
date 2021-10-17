/* ArpRangeControl.h
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
 * 06.12.00		hackborn
 * Created this file.
 */

#ifndef ARPVIEWS_ARPRANGECONTROL_H
#define ARPVIEWS_ARPRANGECONTROL_H

#include <vector>
#include <String.h>
#include <View.h>
class ArpRangeBand;
class _ArpSetValueWindow;
class _ArpValueTextWindow;

#define ARP_ZOOM_CONTROL_WIDTH	(14)
#define ARP_ZOOM_CONTROL_HEIGHT	(14)

// Arbitrary values that define the minimum and maximum ranges
// for the x and y scaling.  These can be changed for a specific
// zoom control with SetRangeX() and SetRangeY()
#define ARP_DEFAULT_MIN_ZOOM	(0.1)
#define ARP_DEFAULT_MAX_ZOOM	(10)

typedef std::vector<ArpRangeBand*>		band_vec;

// Flags for the SetDisplayFlags() method
enum {
	ARP_DISPLAY_RANGE		= 0x00000001,	// By default on, this displays the triangular range indicator
	ARP_DISPLAY_VALUE_BAR	= 0x00000002,	// By default on, this displays a bar that shows the currently selected value
	ARP_DISPLAY_TEXT		= 0x00000004	// By default on, this displays the text window
};

/***************************************************************************
 * ARP-RANGE-CONTROL
 * This is a simple control class that allows users to control the x and
 * y scaling of a target.  It appears as a small icon that is intended to
 * sit next to vertical and horizontal scroll bars.  When the icon is clicked
 * on, a vertical and / or horizontal bar appears, along with a text window.
 * The vertical and horizontal bars allow users to change the scaling rate,
 * the text window is constantly refreshed with the currently selected scales.
 * Returning to the area that the user originally clicked on will return the
 * scaling rate to its original value.
 *
 * If clients only want to scale along a single axis, then in the constructor
 * (or one of the SetRange() functions), the min and max values for the
 * undesired axis should be set to zero.
 *
 * The min values should always be greater than zero.  The zoom value should
 * always be contained within the min and max values.
 ***************************************************************************/
class ArpRangeControl : public BView
{
public:
	ArpRangeControl(BRect frame,
					const char* name,
					uint32 resizeMask,
					float initialX = 1,
					float initialY = 1,
					uint32 displayFlags = ARP_DISPLAY_RANGE | ARP_DISPLAY_VALUE_BAR | ARP_DISPLAY_TEXT);
	virtual ~ArpRangeControl();
	
	uint32				DisplayFlags() const;
	void				SetDisplayFlags(uint32 displayFlags);
	
	/* Add a range band.  On each axis, a new range gets added to the previous
	 * one, so the start value should be slightly higher than the previous stop value.
	 */
	void				AddHorizontalBand(float start, float stop, float pixelSize);
	void				AddHorizontalBand(ArpRangeBand* band);
	const band_vec&		HorizontalBands() const;
	void				AddVerticalBand(float start, float stop, float pixelSize);
	void				AddVerticalBand(ArpRangeBand* band);
	const band_vec&		VerticalBands() const;
	
	/* Helper functions for adding icon bands.
	 */
	void				AddHorizontalIcon(float start, float stop, const BBitmap* image);
	void				AddHorizontalIcon(float start, float stop, const BBitmap* image,
										  float labelStart, float labelStop);
	void				AddVerticalIcon(float start, float stop, const BBitmap* image);
	void				AddVerticalIcon(float start, float stop, const BBitmap* image,
										float labelStart, float labelStop);
	
	/* Helper functions for adding intermediate bands.
	 */
	void				AddHorizontalIntermediate(float start, float stop, float pixelSize);
	void				AddVerticalIntermediate(float start, float stop, float pixelSize);
	
	/* Set this message if you want clients to receive constant update
	 * notices as the value is changing.  Clients can listen to changes
	 * by watching this class with the what value of the UpdatedMessage().
	 * When the message is delivered, it will have two floats added to
	 * it, "value_x" and "value_y", each the current value of the respective
	 * axis.
	 */
	void			SetUpdatedMessage(BMessage* msg);
	BMessage*		UpdatedMessage() const;
	/* Set this message if you want clients to receive an update once
	 * the user is finished setting a new value.  Clients can listen to changes
	 * by watching this class with the what value of the FinishedMessage().
	 * When the message is delivered, it will have two floats added to
	 * it, "value_x" and "value_y", each the current value of the respective
	 * axis.
	 */
	void			SetFinishedMessage(BMessage* msg);
	BMessage*		FinishedMessage() const;	
	
	/* When this control is invoked, it also displays a text view that
	 * shows the actual current value.  Often this control is used with
	 * a scale where 1 is 100% -- and we want that 1 to appear as 100% in
	 * the text view, so by default, the current x and y values are multiplied
	 * by 100 before being displayed.  This method changes that factor.
	 */
	void			SetTextScale(float x = 100, float y = 100);
	/* By default the displayed value is appended with '%' to show that
	 * it is a percentage you can change the prefix and/or suffix to
	 * something else.
	 */
	void			SetTextContext(const char* prefix, const char* suffix);
	/* Set the color used to display the range bar.  By default, it's a
	 * few shades darker than the menu background color.
	 */
	rgb_color		RangeColor() const;
	void			SetRangeColor(rgb_color c);
	
	/* Answer true if this zoom control has a control for zooming
	 * along the given axis, false otherwise.
	 */
	bool			CanZoomX() const;
	bool			CanZoomY() const;

	/* Get and set the current values of the control.
	 */
	float			ZoomX() const;
	float			ZoomY() const;
	void			SetZoomX(float x);
	void			SetZoomY(float y);
	
	virtual	void	Draw(BRect clip);
	virtual	void	MouseDown(BPoint pt);
	virtual	void	MouseUp(BPoint where);
	virtual	void	MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
	virtual	void	GetPreferredSize(float* width, float* height);
	
	/* This method answers true if the triangular range indicator should be
	 * draw large to small, false otherwise.  By default, this is true if
	 * the axis in question starts with its largest values and gets smaller,
	 * otherwise it's false.  Subclasses can override it to change these conditions.
	 */
	virtual bool	LargeToSmall(orientation direction) const;

protected:
	void DrawBorder(BRect clip);

private:
	typedef BView		inherited;
	uint32				mDisplayFlags;
	band_vec			mHorizontals;
	band_vec			mVerticals;
	BMessage*			mUpdatedMsg;
	BMessage*			mFinishedMsg;
	
	// These are the current scale values for the x and y axes.  A value
	// of 1 should be used to mean a 1-to-1 pixel ration.  The values
	// must be greater than 0.
	float				mZoomX, mZoomY;
	
	// The range of allowed scale values on the x and y.  If a min equals
	// a max, then that axis is not allowed to zoom.
	float				mMinX, mMaxX, mMinY, mMaxY;

	/* See the SetTextScale() method for a description.
	 */
	float				mTextX, mTextY;
	/* See the SetTextContext() method for a description.
	 */
	BString				mPrefix, mSuffix;
	/* The color used to display the triangular range bar.
	 */
	rgb_color			mRangeC;
	
	/* These windows display bars with indicators for the current zoom.
	 * The indicators can be dragged to set a new zoom level.
	 */
	_ArpSetValueWindow	*mSetH, *mSetV;
	/* This window displays the text value of the current zoom level.
	 * It is not interactive.
	 */
	_ArpValueTextWindow	*mShowZoom;
	
	// The initial position the user clicked.
	BPoint				mInitPoint;
	
	/* This is set when the mouse is being tracked in the popup windows.
	 */
	bool				mTracking;
	
	/* This is set when the popup windows are running in "sticky" mode.
	 */
	bool				mSticky;
	
	bool DrawLabelFromBands(BRect clip, BView* view);
	void CommitValues();
};

/***************************************************************************
 * ARP-RANGE-BAND
 * This class stores one band of range information for one axis of the
 * range control.
 ***************************************************************************/
class ArpRangeBand
{
public:
	ArpRangeBand(	float start = ARP_DEFAULT_MIN_ZOOM,
					float stop = ARP_DEFAULT_MAX_ZOOM,
					float pixelSize = 10);

	float			mStart, mStop, mPixelSize;

	ArpRangeBand	&operator=(const ArpRangeBand &r);
	/* Answer true if the suppplied value is between my start and stop.
	 */
	bool			Contains(float value) const;
	/* Answer my zoom value for the supplied pixel.
	 */
	float			ZoomFromPixel(float pixel) const;
	/* These methods are used when the band has been bisected by
	 * the no change zone.  They answer the distance from the start
	 * to the bisection, and the bisection to the stop, respectively.
	 */
	float			StartToBisectionPixel(float value) const;
	float			BisectionToStopPixel(float value) const;

	/* Subclasses are given the opportunity to draw something before
	 * the triangular range indicator.  If selected is true, then this
	 * is the band that the user is currently over.
	 */
	virtual void	DrawBackground(BRect frame, BView* view, bool selected);
	/* Subclasses are given the opportunity to draw something on top
	 * of the triangular range indicator.
	 */
	virtual void	DrawForeground(BRect frame, BView* view, float value);
	virtual void	DrawControlForeground(BRect frame, BView* view, float value);
	/* Subclasses can answer if they want this the band to be used as
	 * a label on the range control when the current value is the
	 * value supplied to this method.  By default, the band answers
	 * no, but if any subclass answers yes, its DrawForeground() method
	 * will be called whenever the range control is drawn.
	 */
	virtual bool	UseAsLabel(float value) const;
	/* Alternatively, subclasses can answer that they want this band
	 * to be shown as an "intermediate" between the ones before and
	 * after it, when the current value is the value supplied to this
	 * method.  This only applies if UseAsLabel() returns falls; if
	 * you return true here, the bands before and after it -will- be
	 * drawn, whether they want it or not.  The default is false.
	 */
	virtual bool	UseAsIntermediate(float value) const;
};

/*************************************************************************
 * ARP-BITMAP-RANGE-BAND
 * An ArpRangeBand subclass that draws a bitmap.  The labelStart and
 * labelStop parameters are used to determine when this band should be used
 * for the label.
 *************************************************************************/
class ArpBitmapRangeBand : public ArpRangeBand
{
public:
	ArpBitmapRangeBand(	float start,
						float stop,
						float pixelSize,
						const BBitmap* image,
						float labelStart,
						float labelStop);

	virtual void	DrawBackground(BRect frame, BView* view, bool selected);
	virtual void	DrawForeground(BRect frame, BView* view, float value);
	virtual bool	UseAsLabel(float value) const;

private:
	typedef ArpRangeBand	inherited;
	const BBitmap*			mImage;
	float					mLabelStart, mLabelStop;
};

/*************************************************************************
 * ARP-INTERMEDIATE-RANGE-BAND
 * An ArpRangeBand subclass that allows intermediate values between two
 * other bands.  You will typically place this between two
 * ArpBitmapRangeBand objects, which are set to display a label for two
 * single values, with the ArpIntermediateRangeBand handling all values
 * between them.
 *************************************************************************/
class ArpIntermediateRangeBand : public ArpRangeBand
{
public:
	ArpIntermediateRangeBand(	float start,
								float stop,
								float pixelSize);

	virtual void	DrawBackground(BRect frame, BView* view, bool selected);
	virtual void	DrawControlForeground(BRect frame, BView* view, float value);
	virtual bool	UseAsIntermediate(float value) const;

private:
	typedef ArpRangeBand	inherited;
};

/***************************************************************************
 * ARP-ZOOM-CONTROL
 * The range control started life as just a zoom control -- this is a
 * simple convenience class to provide that original behaviour.
 ***************************************************************************/
class ArpZoomControl : public ArpRangeControl
{
public:
	ArpZoomControl(BRect frame,
					const char* name,
					uint32 resizeMask = B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM,
					float initialX = 1,
					float initialY = 1,
					uint32 displayFlags = ARP_DISPLAY_RANGE | ARP_DISPLAY_VALUE_BAR | ARP_DISPLAY_TEXT);
	virtual ~ArpZoomControl();

	virtual	void	Draw(BRect clip);

private:
	typedef ArpRangeControl	inherited;
};

#endif
