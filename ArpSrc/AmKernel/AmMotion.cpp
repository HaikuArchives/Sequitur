/* AmMotion.cpp
 */
#define _BUILDING_AmKernel 1

#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "AmKernel/AmMotion.h"

static const float		PRECISION				= 0.00001;
static const char*		LABEL_STR				= "label";
static const char*		SHORT_DESCRIPTION_STR	= "short_description";
static const char*		AUTHOR_STR				= "author";
static const char*		EMAIL_STR				= "email";
static const char*		PT_STR					= "pt";
static const char*		END_STR					= "end";
static const char*		EDITING_MODE_STR		= "editing_mode";

/**********************************************************************
 * AM-MOTION-I
 **********************************************************************/
AmMotionI::~AmMotionI()
{
}

AmMotionI* AmMotionI::NewMotion(const BMessage& config)
{
	return new AmMotion(config, false, NULL);
}

/**********************************************************************
 * AM-MOTION
 **********************************************************************/
AmMotion::AmMotion(const char* author, const char* email)
		: mAuthor(author), mEmail(email), mIsValid(true),
		  mEditingMode(RHYTHM_MODE), mReadOnly(false)
{
	mSignatures.SetSignature(1, 4, 4);
}

AmMotion::AmMotion(	const BString& label, const char* author,
					const char* email, const char* filePath)
		: mLabel(label),  mAuthor(author), mEmail(email),
		  mIsValid(true), mEditingMode(ENVELOPE_MODE), mReadOnly(false),
		  mFilePath(filePath)
{
	mSignatures.SetSignature(1, 4, 4);
}

AmMotion::AmMotion(const AmMotionI& o)
		: mIsValid(true), mEditingMode(ENVELOPE_MODE), mReadOnly(false)
{
	mLabel = o.Label();
	const AmMotion*	m = dynamic_cast<const AmMotion*>(&o);
	if (m) {
 		mShortDescription = m->mShortDescription;
 		mAuthor = m->mAuthor;
		mEmail = m->mEmail;
		for (uint32 k = 0; k < m->mHits.size(); k++)
			mHits.push_back( _AmMotionHit(m->mHits[k]) );
		mSignatures = m->mSignatures;
		mEditingMode = m->mEditingMode;
		mFilePath = m->mFilePath;
	}
	if (mSignatures.IsEmpty() ) mSignatures.SetSignature(1, 4, 4);
}

AmMotion::AmMotion(const BMessage& config, bool readOnly, const char* filePath)
		: mIsValid(true), mEditingMode(ENVELOPE_MODE), mReadOnly(readOnly),
		  mFilePath(filePath)
{
	ReadFrom(config);
	if (mSignatures.IsEmpty() ) mSignatures.SetSignature(1, 4, 4);
}

AmMotion::~AmMotion()
{
	mSignatures.DeleteEvents();
}

file_entry_id AmMotion::Id() const
{
	return (void*)this;
}

BString AmMotion::Label() const
{
	return mLabel;
}

BString AmMotion::Key() const
{
	return mLabel;
}

BString AmMotion::ShortDescription() const
{
	return mShortDescription;
}

BString AmMotion::Author() const
{
	return mAuthor;
}

BString AmMotion::Email() const
{
	return mEmail;
}

bool AmMotion::IsValid() const
{
	return mIsValid;
}

uint32 AmMotion::CountHits() const
{
	return mHits.size();
}

uint32 AmMotion::CountMeasures() const
{
	if (mHits.size() < 1) return 0;
	uint32		i = mHits.size() - 1;
	float		end = (mHits[i].HasEnd() ) ? mHits[i].mEnd : mHits[i].mPt.x;
	return uint32(floor(end)) + 1;
}

status_t AmMotion::GetHit(	uint32 number,
							BPoint* pt, float* end) const
{
	ArpASSERT(pt);
	if (number >= mHits.size() ) return B_ERROR;
	*pt = mHits[number].mPt;
	if (end) {
		if (mHits[number].HasEnd() ) *end = mHits[number].mEnd;
		else if (number + 1 < mHits.size() ) *end = mHits[number + 1].mPt.x - PRECISION;
		else {
			float		m = floor(pt->x);
			*end = m + 0.999999999;
		}
	}
	return B_OK;
}

static inline float interpolated_y(float x, BPoint fromPt, BPoint toPt)
{
	if (x <= fromPt.x) return fromPt.y;
	if (x >= toPt.x) return toPt.y;
	
	float		minY = min(fromPt.y, toPt.y);
	float		maxY = max(fromPt.y, toPt.y);
	float		distX = toPt.x - fromPt.x;
	float		distY = maxY - minY;
	
	float		y = ( (x - fromPt.x) * distY) / distX;
	if (fromPt.y > toPt.y) y = fromPt.y - y;
	else y = fromPt.y + y;
	if (y < minY) y = minY;
	else if (y > maxY) y = maxY;

	return y;
}

status_t AmMotion::GetHitY(float x, float* outY) const
{
	for (uint32 k = 0; k < mHits.size(); k++) {
		if (x >= mHits[k].mPt.x) {
			if (mEditingMode != ENVELOPE_MODE) {
				float		end = x;
				if (mHits[k].HasEnd() ) end = mHits[k].mEnd;
				if (x <= end) {
					*outY = mHits[k].mPt.y;
					return B_OK;
				}
			} else {
				if (k + 1 >= mHits.size() ) return B_ERROR;
				if (x <= mHits[k + 1].mPt.x) {
					*outY = interpolated_y(x, mHits[k].mPt, mHits[k + 1].mPt);
					return B_OK;
				}
			}
		}
	}
	return B_ERROR;
}

bool AmMotion::IsEmpty() const
{
	return mHits.size() == 0;
}

AmMotionMode AmMotion::EditingMode() const
{
	return mEditingMode;
}

AmMotionI* AmMotion::Copy() const
{
	return new AmMotion(*this);
}

bool AmMotion::IsReadOnly() const
{
	return mReadOnly;
}

void AmMotion::SetLabel(const BString& label)
{
	mLabel = label;
}

void AmMotion::SetShortDescription(const char* s)
{
	mShortDescription = s;
}

void AmMotion::SetAuthor(const char* author)
{
	mAuthor = author;
}

void AmMotion::SetEmail(const char* email)
{
	mEmail = email;
}

void AmMotion::SetIsValid(bool isValid)
{
	mIsValid = isValid;
}

void AmMotion::AddHit(BPoint pt, float end)
{
	ArpASSERT(pt.x >= 0 && (end < 0 || end >= pt.x));
	ArpASSERT(pt.y >= -1 && pt.y <= 1);
	if (mHits.size() > 0) {
		ArpASSERT(pt.x >= mHits[mHits.size()-1].mPt.x);
		float	lastEnd = mHits[mHits.size()-1].mEnd;
		if (lastEnd >= pt.x)
			mHits[mHits.size()-1].mEnd = pt.x - PRECISION;
	}
	mHits.push_back( _AmMotionHit(pt, end) );
}

const AmSignaturePhrase& AmMotion::Signatures() const
{
	return mSignatures;
}

void AmMotion::SetSignatures(const AmSignaturePhrase& signatures)
{
	mSignatures.DeleteEvents();
	mSignatures = signatures;
	if (mSignatures.IsEmpty() ) mSignatures.SetSignature(1, 4, 4);
}

void AmMotion::SetEditingMode(AmMotionMode mode)
{
	mEditingMode = mode;
}

status_t AmMotion::WriteTo(BMessage& config) const
{
	status_t	err;
	if ((err=config.AddString(LABEL_STR, mLabel)) != B_OK) return err;
	if (mShortDescription.Length() > 0)
		config.AddString(SHORT_DESCRIPTION_STR, mShortDescription);
	if (mAuthor.Length() > 0)
		if ((err=config.AddString(AUTHOR_STR, mAuthor)) != B_OK) return err;
	if (mEmail.Length() > 0)
		if ((err=config.AddString(EMAIL_STR, mEmail)) != B_OK) return err;
	if ((err=config.AddInt32(EDITING_MODE_STR, int32(mEditingMode) )) != B_OK) return err;
	for (uint32 k = 0; k < mHits.size(); k++) {
		if ((err=config.AddPoint(PT_STR, mHits[k].mPt)) != B_OK) return err;
		if ((err=config.AddFloat(END_STR, mHits[k].mEnd)) != B_OK) return err;
	}
	return B_OK;
}

status_t AmMotion::ReadFrom(const BMessage& config)
{
	const char*		str;
	if (config.FindString(LABEL_STR, &str) == B_OK) mLabel = str;
	if (config.FindString(SHORT_DESCRIPTION_STR, &str) == B_OK) mShortDescription = str;
	if (config.FindString(AUTHOR_STR, &str) == B_OK) mAuthor = str;
	if (config.FindString(EMAIL_STR, &str) == B_OK) mEmail = str;
	int32			i;
	if (config.FindInt32(EDITING_MODE_STR, &i) == B_OK) mEditingMode = AmMotionMode(i);
	
	BPoint			pt;
	for (int32 k = 0; config.FindPoint(PT_STR, k, &pt) == B_OK; k++) {
		float		f;
		status_t	err;
		if ((err=config.FindFloat(END_STR, k, &f)) != B_OK) return err;
		mHits.push_back( _AmMotionHit(pt, f) );
	}
	return B_OK;
}


BString AmMotion::LocalFileName() const
{
	return convert_to_filename(mLabel);
}

BString AmMotion::LocalFilePath() const
{
	return mFilePath;
}

AmMotion& AmMotion::operator=(const AmMotion& o)
{
	mLabel = o.mLabel;
	mAuthor = o.mAuthor;
	mEmail = o.mEmail;
	mIsValid = o.mIsValid;
	mEditingMode = o.mEditingMode;
	mReadOnly = o.mReadOnly;
	mFilePath = o.mFilePath;
	mHits.resize(0);
	for (uint32 k = 0; k < o.mHits.size(); k++) {
		mHits.push_back( o.mHits[k] );
	}
	return *this;
}

void AmMotion::Print()
{
	printf("AmMotion %s\n", mLabel.String() );
	for (uint32 k = 0; k < mHits.size(); k++) {
		printf("\thit: %f, %f end %f\n", mHits[k].mPt.x,
				mHits[k].mPt.y, mHits[k].mEnd);
	}
}

/**********************************************************************
 * _AM-MOTION-HIT
 **********************************************************************/
_AmMotionHit::_AmMotionHit()
		: mPt(0, 0), mEnd(-1)
{
}

_AmMotionHit::_AmMotionHit(BPoint pt, float end)
		: mPt(pt), mEnd(end)
{
}

_AmMotionHit::_AmMotionHit(const _AmMotionHit& o)
		: mPt(o.mPt), mEnd(o.mEnd)
{
}

_AmMotionHit& _AmMotionHit::operator=(const _AmMotionHit& o)
{
	mPt = o.mPt;
	mEnd = o.mEnd;
	return *this;
}

bool _AmMotionHit::HasEnd() const
{
	return mEnd >= 0;
}
