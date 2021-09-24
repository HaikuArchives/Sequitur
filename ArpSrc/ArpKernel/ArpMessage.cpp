/*
	
	ArpMessage.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

enum {
	// This is our own custom type.  It indicates that the value for this field
	// is stored in some other "global" message, under the given name.  The value is
	// actually stored as a \0-terminated string.
	// the meaning of what a "global" message is and where to find it is undefined.
	ARP_INDIRECT_TYPE = 'INDr'
};

#if 0
// This is to get at the ARP_INDIRECT_TYPE definition.
#ifndef ARPLAYOUT_ARPPARAM_H
#include <ArpLayout/ArpParam.h>
#endif
#endif

#ifndef _GRAPHICS_DEFS_H
#include <GraphicsDefs.h>
#endif

#ifndef  _ENTRY_H
#include <storage/Entry.h>
#endif

#ifndef _MESSENGER_H
#include <app/Messenger.h>
#endif

#include <cstring>

ArpMOD();

#define myinline

BMessage& ArpUpdateMessage(BMessage& to, const BMessage& msg)
{
	char* name;
	type_code type;
	int32 count;
	for( int32 i=0; !msg.GetInfo(B_ANY_TYPE,i,&name,&type,&count);
		i++ ) {
		bool all_gone = false;
		for( int32 j=0; j<count; j++ ) {
			type_code dummy;
			bool fixedSize=true;
			msg.GetInfo(name, &dummy, &fixedSize);
			const void* data;
			ssize_t size;
			if( !msg.FindData(name,type,j,&data,&size) ) {
				if( !all_gone ) {
					if( type == B_MESSAGE_TYPE ) {
						BMessage oldMsg;
						BMessage newMsg;
						if( !to.FindMessage(name,j,&oldMsg) &&
							!msg.FindMessage(name,j,&newMsg) ) {
							ArpUpdateMessage(oldMsg, newMsg);
							to.ReplaceMessage(name,j,&oldMsg);
						} else {
							all_gone = true;
						}
					}
					if( to.ReplaceData(name,type,j,data,size) ) {
						int32 cnt=0;
						type_code mtype = type;
						if( !to.GetInfo(name,&mtype,&cnt) ) {
							for( int32 k=cnt-1; k>=j; k-- ) {
								to.RemoveData(name,k);
							}
						}
						all_gone = true;
					}
				}
				if( all_gone ) {
					to.AddData(name,type,data,size,fixedSize);
				}
			}
		}
	}
	return to;
}

ArpMessage&	ArpMessage::Update(const BMessage& msg)
{
	if( this ) ArpUpdateMessage(*this, msg);
	
#if 0
	if( this ) {
		char* name;
		type_code type;
		long count;
		for( int32 i=0; !msg.GetInfo(B_ANY_TYPE,i,&name,&type,&count);
			i++ ) {
			bool all_gone = false;
			for( int32 j=0; j<count; j++ ) {
				const void* data;
				ssize_t size;
				if( !msg.FindData(name,type,j,&data,&size) ) {
					if( !all_gone &&
						ReplaceData(name,type,j,data,size) ) {
						long cnt=0;
						type_code mtype = type;
						if( !GetInfo(name,&mtype,&cnt) ) {
							for( int32 k=cnt-1; k>=j; k-- ) {
								RemoveData(name,k);
							}
						}
						all_gone = true;
					}
					if( all_gone ) {
						AddData(name,type,data,size);
					}
				}
			}
		}
	}
#endif

	return *this;
}

status_t ArpMessage::AddRGBColor(const char *name, const rgb_color *col)
{
	if( !col ) return B_BAD_VALUE;
	return AddData(name,B_RGB_COLOR_TYPE,col,sizeof(*col));
}

status_t ArpMessage::FindRGBColor(const char *name, int32 index, rgb_color *col) const
{
	const rgb_color* mdat = NULL;
	ssize_t msize = 0;
	status_t res = FindData(name,B_RGB_COLOR_TYPE,index,(const void**)&mdat,&msize);
	if( res ) return res;
	if( !mdat || msize != sizeof(*mdat) ) {
		return B_NO_INIT;
	}
	*col = *mdat;
	return B_NO_ERROR;
}

status_t ArpMessage::FindRGBColor(const char *name, rgb_color *col) const
{
	return FindRGBColor(name,(int32)0,col);
}

bool ArpMessage::HasRGBColor(const char* name, int32 index) const
{
	return HasData(name, B_RGB_COLOR_TYPE, index);
}

status_t ArpMessage::ReplaceRGBColor(const char *name, const rgb_color *col)
{
	if( !col ) return B_BAD_VALUE;
	return ReplaceData(name,B_RGB_COLOR_TYPE,col,sizeof(*col));
}

status_t ArpMessage::ReplaceRGBColor(const char *name, int32 index, const rgb_color *col)
{
	if( !col ) return B_BAD_VALUE;
	return ReplaceData(name,B_RGB_COLOR_TYPE,index,col,sizeof(*col));
}

status_t ArpMessage::AddFont(const char *name, const BFont *font)
{
	return AddMessageFont(this, name, font);
}

status_t ArpMessage::FindFont(const char *name, int32 index, BFont *font) const
{
	return FindMessageFont(this, name, index, font);
}

status_t ArpMessage::FindFont(const char *name, BFont *font) const
{
	return FindFont(name,0,font);
}

bool ArpMessage::HasFont(const char* name, int32 index) const
{
	return HasData(name, FFont::FONT_TYPE, index);
}

status_t ArpMessage::ReplaceFont(const char *name, int32 index, const BFont *font)
{
	return B_ERROR; // not yet implemented
}

status_t ArpMessage::ReplaceFont(const char *name, const BFont *font)
{
	return B_ERROR; // not yet implemented
}

status_t ArpMessage::AddIndirect(const char *name, const char* field)
{
	if( !field ) return B_BAD_VALUE;
	return AddData(name, ARP_INDIRECT_TYPE, field, strlen(field)+1);
}

status_t ArpMessage::FindIndirect(const char *name, int32 index, const char** ind) const
{
	const char* mdat = 0;
	ssize_t msize = 0;
	status_t res = FindData(name,ARP_INDIRECT_TYPE,index,(const void**)&mdat,&msize);
	if( res ) return res;
	if( !mdat || mdat[msize-1] != '\0' ) {
		return B_NO_INIT;
	}
	*ind = mdat;
	return B_NO_ERROR;
}

status_t ArpMessage::FindIndirect(const char *name, const char** ind) const
{
	return FindIndirect(name,(int32)0,ind);
}

bool ArpMessage::HasIndirect(const char* name, int32 index) const
{
	return HasData(name, B_RGB_COLOR_TYPE, index);
}

status_t ArpMessage::ReplaceIndirect(const char *name, const char* ind)
{
	if( !ind ) return B_BAD_VALUE;
	return ReplaceData(name,ARP_INDIRECT_TYPE,ind,strlen(ind)+1);
}

status_t ArpMessage::ReplaceIndirect(const char *name, int32 index, const char* ind)
{
	if( !ind ) return B_BAD_VALUE;
	return ReplaceData(name,ARP_INDIRECT_TYPE,index,ind,strlen(ind)+1);
}

/* ------------------------------------------------------------- */

myinline ArpMessage& ArpMessage::SetRect(const char *name, const BRect& dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddRect(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetPoint(const char *name, const BPoint& dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddPoint(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetString(const char *name, const char* dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddString(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetInt8(const char *name, int8 dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddInt8(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetInt16(const char *name, int16 dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddInt16(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetInt32(const char *name, int32 dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddInt32(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetInt64(const char *name, int64 dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddInt64(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetBool(const char *name, bool dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddBool(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetFloat(const char *name, float dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddFloat(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetDouble(const char *name, double dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddDouble(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetPointer(const char *name, const void* dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddPointer(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetMessenger(const char *name, const BMessenger& dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddMessenger(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetRef(const char *name, const entry_ref* dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddRef(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetMessage(const char *name, const BMessage* dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddMessage(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetRGBColor(const char *name, const rgb_color* dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddRGBColor(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetFont(const char *name, const BFont* dat)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddFont(name, dat);
	}
	return *this;
}

myinline ArpMessage& ArpMessage::SetIndirect(const char *name, const char* field)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddIndirect(name, field);
	}
	return *this;
#if 0
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddData(name, ARP_INDIRECT_TYPE, field, strlen(field)+1);
		//printf("Set indirect: %ls\n", field);
	}
	return *this;
#endif
}

myinline ArpMessage& ArpMessage::SetData(const char *name,
										type_code type,
										const void *data,
										ssize_t numBytes)
{
	if( !this ) return *((ArpMessage*)NULL);
	if( status == B_NO_ERROR ) {
		status = AddData(name, type, data, numBytes);
	}
	return *this;
}

/* ------------------------------------------------------------- */

myinline const BRect ArpMessage::GetRect(const char *name, const BRect& def, int32 index) const
{
	if( this ) {
		BRect res;
		if( FindRect(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline const BPoint ArpMessage::GetPoint(const char *name, const BPoint& def, int32 index) const
{
	if( this ) {
		BPoint res;
		if( FindPoint(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline const char* ArpMessage::GetString(const char *name, const char* def, int32 index) const
{
	if( this ) {
		const char* res;
		if( FindString(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline int8 ArpMessage::GetInt8(const char *name, int8 def, int32 index) const
{
	if( this ) {
		int8 res;
		if( FindInt8(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline int16 ArpMessage::GetInt16(const char *name, int16 def, int32 index) const
{
	if( this ) {
		int16 res;
		if( FindInt16(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline int32 ArpMessage::GetInt32(const char *name, int32 def, int32 index) const
{
	if( this ) {
		int32 res;
		if( FindInt32(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline int64 ArpMessage::GetInt64(const char *name, int64 def, int32 index) const
{
	if( this ) {
		int64 res;
		if( FindInt64(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline bool ArpMessage::GetBool(const char *name, bool def, int32 index) const
{
	if( this ) {
		bool res;
		if( FindBool(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline float ArpMessage::GetFloat(const char *name, float def, int32 index) const
{
	if( this ) {
		float res;
		if( FindFloat(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline double ArpMessage::GetDouble(const char *name, double def, int32 index) const
{
	if( this ) {
		double res;
		if( FindDouble(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline void* ArpMessage::GetPointer(const char *name, void* def, int32 index) const
{
	if( this ) {
		void* res;
		if( FindPointer(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline const BMessenger ArpMessage::GetMessenger(const char *name, const BMessenger& def, int32 index) const
{
	if( this ) {
		BMessenger res;
		if( FindMessenger(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline const entry_ref ArpMessage::GetRef(const char *name, const entry_ref& def, int32 index) const
{
	if( this ) {
		entry_ref res;
		if( FindRef(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline const BMessage ArpMessage::GetMessage(const char *name, const BMessage& def, int32 index) const
{
	if( this ) {
		BMessage res;
		if( FindMessage(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline const rgb_color ArpMessage::GetRGBColor(const char *name, const rgb_color& def, int32 index) const
{
	if( this ) {
		rgb_color res;
		if( FindRGBColor(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline const BFont ArpMessage::GetFont(const char *name, const BFont& def, int32 index) const
{
	if( this ) {
		BFont res;
		if( FindFont(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

myinline const char* ArpMessage::GetIndirect(const char *name, const char* def, int32 index) const
{
	if( this ) {
		const char* res;
		if( FindIndirect(name, index, &res) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
#if 0
	if( this ) {
		const char* res = NULL;
		ssize_t bytes = 0;
		if( FindData(name, ARP_INDIRECT_TYPE, index, (const void**)&res, &bytes) != B_NO_ERROR ) return def;
		if( !res ) return def;
		ArpD(cdb << ADH << "Got indirect: " << res << " for " << name
					<< " (" << bytes << " bytes)" << std::endl);
		if( res[bytes-1] != '\0' ) {
			printf("Oops!\n");
			return def;
		}
		return res;
	}
	return def;
#endif
}

myinline const void* ArpMessage::GetData(const char *name, type_code type,
										ssize_t* gotBytes,
										const void* def, int32 index) const
{
	if( this ) {
		const void* res;
		if( FindData(name, type, index, &res, gotBytes) != B_NO_ERROR ) return def;
		return res;
	}
	return def;
}

