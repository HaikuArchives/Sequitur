/*******************************************************************************
/
/	File:			ResourceSet.h
/
/   Description:    Experimental class for managing resources.
/
*******************************************************************************/

#ifndef RESOURCE_SET_H
#define RESOURCE_SET_H

#ifndef _BITMAP_H
#include <Bitmap.h>
#endif

#ifndef _CURSOR_H
#include <Cursor.h>
#endif

#ifndef _LIST_H
#include <List.h>
#endif

#ifndef _LOCKER_H
#include <Locker.h>
#endif

#ifndef _MESSAGE_H
#include <Message.h>
#endif

#ifndef _SUPPORT_DEFS_H
#include <SupportDefs.h>
#endif

class BResources;
class BDataIO;

namespace BExperimental {

namespace BResourcePrivate {
	class TypeItem;
	class TypeList;
}

using namespace BResourcePrivate;

// Some new types we handle.
enum {
	B_CURSOR_TYPE			= 'CURS',
	B_BITMAP_TYPE			= 'BBMP'
};

// String conversion.  The default implementation does a
// getenv() on the requested name.
class BStringMap
{
public:
	BStringMap();
	virtual ~BStringMap();
	
	virtual const char* FindString(const char* name);
};

class BResourceSet
{
public:
	BResourceSet();
	virtual ~BResourceSet();

	// Add resource files that are searched for data.  Resources are
	// checked from first to last; the first one that has a match for
	// a particular request is used for that data.
	
	status_t AddResources(BResources* resources, bool at_front=true);
	status_t AddResources(const void* image_addr, bool at_front=true);
	
	// Add directories that are searched for data.  Directories are
	// always searched before resource files; they are only searched
	// when looking for a resource by name.
	
	status_t AddDirectory(const char* full_path, bool at_front=true);
	status_t AddEnvDirectory(const char* env_path,
							 const char* default_value = 0,
							 bool at_front=true,
							 BStringMap* variables=0);
	
	// Functions to look up generic resource data.
	
	const void* FindResource(type_code type, int32 id,
							 size_t* out_size);
	const void* FindResource(type_code type, const char* name,
							 size_t* out_size);
	
	// Functions to look up specific types of data.
	
	const BBitmap* FindBitmap(type_code type, int32 id);
	const BBitmap* FindBitmap(type_code type, const char* name);
	
	const BCursor* FindCursor(type_code type, int32 id);
	const BCursor* FindCursor(type_code type, const char* name);
	
	const BMessage* FindMessage(type_code type, int32 id);
	const BMessage* FindMessage(type_code type, const char* name);
	
	// Functions to look up specific types of data with standard type codes.
	
	const BBitmap* FindBitmap(int32 id);
	const BBitmap* FindBitmap(const char* name);
	
	const BCursor* FindCursor(int32 id);
	const BCursor* FindCursor(const char* name);
	
	const BMessage* FindMessage(int32 id);
	const BMessage* FindMessage(const char* name);

protected:
	class TypeObject {
	public:
		TypeObject();
		virtual ~TypeObject();
		void Delete();
		
	private:
		TypeObject(const TypeObject& o);
		TypeObject& operator=(const TypeObject& o);
		bool operator==(const TypeObject& o);
		bool operator!=(const TypeObject& o);
		
		bool fDeleteOK;
	};
	
	class BitmapObject : public BBitmap, public TypeObject
	{
	public:
		BitmapObject(BRect bounds,
						uint32 flags,
						color_space depth,
						int32 bytesPerRow=B_ANY_BYTES_PER_ROW,
						screen_id screenID=B_MAIN_SCREEN_ID);
		BitmapObject(const BBitmap* source,
						bool accepts_views = false,
						bool need_contiguous = false);
		BitmapObject(BMessage *data);
		
		virtual ~BitmapObject();
	};
	
	class CursorObject : public BCursor, public TypeObject
	{
	public:
		CursorObject(const void* data);
		CursorObject(BMessage *data);
		virtual ~CursorObject();
	};
	
	class MessageObject : public BMessage, public TypeObject
	{
	public:
		MessageObject();
		virtual ~MessageObject();
	};
	
	virtual BitmapObject* GenerateBitmap(const void* data, size_t size);
	virtual CursorObject* GenerateCursor(const void* data, size_t size);
	virtual MessageObject* GenerateMessage(const void* data, size_t size);
	
private:
	friend class BResourcePrivate::TypeItem;
	
	status_t expand_string(BString* out, const char* in,
						   BStringMap* variables);
	TypeList* find_type_list(type_code type);
	
	TypeItem* find_item_id(type_code type, int32 id);
	TypeItem* find_item_name(type_code type, const char* name);
	
	TypeItem* load_resource(type_code type, int32 id, const char* name,
							TypeList** inout_list = 0);
	
	BBitmap* return_bitmap_item(type_code type, TypeItem* from);
	BCursor* return_cursor_item(TypeItem* from);
	BMessage* return_message_item(TypeItem* from);
	
	static BitmapObject* read_png_image(BDataIO* stream);
	
	BLocker fLock;				// access control.
	BList fResources;			// containing BResources objects.
	BList fDirectories;			// containing BPath objects.
	BList fTypes;				// containing TypeList objects.
};

}	// namespace BExperimental

using namespace BExperimental;

#endif
