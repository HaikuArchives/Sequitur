/* AmMotion.h
 * Copyright (c)2001 by Angry Red Planet and Eric Hackborn.
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
 * 2001.04.26		hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef AMKERNEL_AMMOTION_H
#define AMKERNEL_AMMOTION_H

#include <vector.h>
#include <app/Message.h>
#include "AmPublic/AmMotionI.h"
#include "AmKernel/AmFileRosterEntryI.h"
#include "AmKernel/AmPhrase.h"

/**********************************************************************
 * _AM-MOTION-HIT
 **********************************************************************/
class _AmMotionHit
{
public:
	_AmMotionHit();
	_AmMotionHit(BPoint pt, float end = -1);
	_AmMotionHit(const _AmMotionHit& o);

	_AmMotionHit&	operator=(const _AmMotionHit& o);

	bool			HasEnd() const;

	/* mPt.X will be 0 - 0.9999... for the first measure,
	 * 1 - 1.9999..., second measure, etc.  mEnd will be
	 * either -1 to indicate there's no end, or in the same
	 * range (but >=) mPt.x.  mPt.y will be -1 to 1.
	 */
	BPoint			mPt;
	float			mEnd;
};

/**********************************************************************
 * AM-MOTION
 **********************************************************************/
class AmMotion : public AmMotionI,
				 public AmFileRosterEntryI
{
public:
	AmMotion(const char* author = NULL, const char* email = NULL);
	AmMotion(	const BString& label, const char* author = NULL,
				const char* email = NULL, const char* filePath = NULL);
	AmMotion(const AmMotionI& o);
	AmMotion(const BMessage& config, bool readOnly, const char* filePath);
	virtual ~AmMotion();

	// --------------------------------------------------------
	// AM-MOTION-I INTERFACE
	// --------------------------------------------------------
	virtual uint32			CountHits() const;
	virtual uint32			CountMeasures() const;
	virtual status_t		GetHit(	uint32 number,
									BPoint* pt, float* end = NULL) const;
	virtual status_t		GetHitY(float x, float* outY) const;
	virtual bool			IsEmpty() const;
	virtual AmMotionMode	EditingMode() const;
	virtual AmMotionI*		Copy() const;

	// --------------------------------------------------------
	// AM-FILE-ROSTER-ENTRY-I INTERFACE
	// --------------------------------------------------------
	virtual file_entry_id	Id() const;
	virtual BString			Label() const;
	virtual BString			ShortDescription() const;
	virtual BString			Key() const;
	virtual BString			Author() const;
	virtual BString			Email() const;
	virtual bool			IsValid() const;
	virtual BString			LocalFileName() const;
	virtual BString			LocalFilePath() const;
	virtual status_t		WriteTo(BMessage& config) const;

	// --------------------------------------------------------
	// MODIFICATION
	// --------------------------------------------------------
	bool					IsReadOnly() const;
	void					SetLabel(const BString& label);
	void					SetShortDescription(const char* s);
	void					SetAuthor(const char* author);
	void					SetEmail(const char* email);
	void					SetIsValid(bool isValid);
	void					AddHit(BPoint pt, float end = -1);
	const AmSignaturePhrase& Signatures() const;
	void					SetSignatures(const AmSignaturePhrase& signatures);
	void					SetEditingMode(AmMotionMode mode);
	
	status_t				ReadFrom(const BMessage& config);
	AmMotion&				operator=(const AmMotion& o);
	void					Print();

private:
	typedef AmMotionI		inherited;
	BString					mLabel;
	BString					mShortDescription;
	BString					mAuthor;
	BString			 		mEmail;
	bool					mIsValid;
	vector<_AmMotionHit>	mHits;
	AmSignaturePhrase		mSignatures;
	AmMotionMode			mEditingMode;
	bool					mReadOnly;
	BString					mFilePath;
};

#endif 

