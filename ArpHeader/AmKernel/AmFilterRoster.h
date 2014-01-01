/* AmFilterRoster.h
 * Copyright (c)2000 by Angry Red Planet.
 * All rights reserved.
 *
 * Author: Dianne Hackborn
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Angry Red Planet,
 * at <hackborn@angryredplanet.com> or <hackbod@angryredplanet.com>.
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
 * 2000/06/18		hackbod
 * Created this file
 */

#ifndef AMKERNEL_AMFILTERROSTER_H
#define AMKERNEL_AMFILTERROSTER_H

#ifndef _ADD_ON_MANAGER_H
#include <AddOnManager.h>
#endif

#include <String.h>

#include "AmPublic/AmFilterI.h"

/* String constant for including raw filter pointers
 * in a message.  Note that you must include both of these --
 * only the filter's owner can safely do anything with
 * the filter pointer.
 */
//extern const char* SZ_FILTER_ADD_ON;

/* Messages used when dragging filter addons around.
 */
enum {
	ARPMSG_FILTERADDONDRAG				= 'aFad'
	// SZ_FILTER_NAME				The filter's public name
	// SZ_FILTER_KEY				The class identifier for the filter
	// SZ_FILTER_SORT_DESCRIPTION	English description of filter
	// SZ_FILTER_TYPE				Filter type
	// SZ_FILTER_SUBTYPE			Filter subtype
	// SZ_FILTER_IMAGE				Flattened bitmap of filter icon
	// SZ_FILTER_CONFIG_TEMPLATE	Additional specification of filter config
	
	// SZ_FILTER_ADD_ON				A pointer to an AmFilterAddOn
	
	// "buttons"					An int32, a mask for the buttons depressed when dragging was initiated
	// "filter_id"					A pointer, the ID of the filter being dragged
	// SZ_TRACK_ID					A pointer, the track that owns the original filter (if any)
	// SZ_SONG_ID					A pointer, the song that owns the track, if any
	// "pipeline type"				An int32, the pipeline the original filter is from
};


class AmFilterAddOnHandle : public BAddOnHandle
{
public:
	// This constructor creates a handle for an add-on that exists in
	// a file.  The image will be loaded and the AmFilterAddOn instance
	// created, as needed; when the AmFilterAddOn gets deleted, it will
	// inform this class so that it can unload the image if needed.
	AmFilterAddOnHandle(const entry_ref* ref, const node_ref* node);
	
	// This constructor creates a handle for an add-on that is linked
	// into the application.  The handle will hold a reference on the
	// add-on, but the add-on won't hold a reference on the handle --
	// the when the handle gets released, it can remove the reference
	// on the add-on to let it be deleted when okay.
	// Note that the add-on instance you pass in here should be created
	// with a NULL cookie!
	AmFilterAddOnHandle(AmFilterAddOn* addOn);
	
	// Information about this add-on, just like in the AmFilterAddon
	// interface.  (So look there for what these mean.)
	BString Name() const;
	BString Key() const;
	int32	MaxConnections() const;
	BString ShortDescription() const;
	void	LongDescription(BString& name, BString& str) const;
	BString Author() const;
	BString Email() const;
	void	GetVersion(int32* major, int32* minor) const;
	AmFilterAddOn::type Type() const;
	AmFilterAddOn::subtype Subtype() const;
	BString KeyForType(AmFilterAddOn::type inType) const;
	BBitmap* RawImage(BPoint requestedSize) const;		// returns a COPY.
	BBitmap* Image(BPoint requestedSize) const;		// returns a COPY.
	float CheckInstantiation(const BMessage* archive) const;
	status_t GetArchiveTemplate(BMessage* into) const;
	
	BBitmap* FinalImage(BPoint requestedSize) const;
	void SetTint(rgb_color tint);
	rgb_color Tint() const;
	
	ArpRef<AmFilterAddOn> Instantiate();
	
	virtual bool KeepLoaded() const;
	virtual bool IsDynamic() const;
	virtual size_t GetMemoryUsage() const;

	// Return the add on instance.  Returns NULL if it is not
	// yet loaded.
	AmFilterAddOn* AddOn() const;
	
protected:
	~AmFilterAddOnHandle();
	
	virtual void ImageLoaded(image_id image);
	virtual status_t LoadIdentifiers(BMessage* into, image_id from);
	virtual void ImageUnloading(image_id image);
	virtual const char* AttrBaseName() const;

private:
	friend class	AmFilterAddOn;
	
	bool			DeleteFilterAddOn();
	
	BBitmap*		mImage;
	rgb_color		mTint;
	AmFilterAddOn*	mAddOn;
};

class AmFilterRosterHandler;

class AmFilterRoster : public BAddOnManager
{
public:
	AmFilterRoster(const char* name);
	virtual ~AmFilterRoster();

	static AmFilterRoster* Default();
	static void ShutdownDefault(bool force_unload=false);
	
	virtual status_t Run();

	virtual void InstallAddOn(BAddOnHandle* addon);

	ArpRef<AmFilterAddOn> FindFilterAddOn(const BMessage* archive) const;
	ArpRef<AmFilterAddOn> FindFilterAddOn(const char* key) const;
	AmFilterI* InstantiateFilter(AmFilterHolderI* holder,
								 const BMessage* archive) const;
	
protected:
	virtual	BAddOnHandle* InstantiateHandle(const entry_ref* entry,
											const node_ref* node);

private:
	AmFilterRosterHandler* mHandler;
};

#endif
