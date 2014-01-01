/*
	Copyright 2000, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <BeBuild.h>

namespace BExperimental {
class _EXPORT BResourceSet;
}

#define USE_RESOURCES 1
#define HAVE_CURSORS 1

#include <be/experimental/ResourceSet.h>

#if HAVE_CURSORS
#include <Cursor.h>
#endif

#include <Bitmap.h>

#include <Entry.h>
#include <File.h>
#include <Path.h>

#if USE_RESOURCES
#include <Resources.h>
#endif

#include <Autolock.h>
#include <DataIO.h>
#include <Debug.h>
#include <String.h>

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <png.h>

namespace BExperimental {
namespace BResourcePrivate {

	class TypeItem {
	public:
		TypeItem(int32 id, const char* name, const void* data, size_t size)
			: fID(id), fName(name),
			  fData(const_cast<void*>(data)), fSize(size), fObject(0),
			  fOwnData(false), fSourceIsFile(false)
		{
		}
		TypeItem(int32 id, const char* name, BFile* file)
			: fID(id), fName(name), fData(0), fSize(0), fObject(0),
			  fOwnData(true), fSourceIsFile(false)
		{
			off_t size;
			if( file->GetSize(&size) == B_OK ) {
				fSize = (size_t)size;
				fData = malloc(fSize);
				if( file->ReadAt(0, fData, fSize) < B_OK ) {
					free(fData);
					fData = 0;
					fSize = 0;
				}
			}
		}
		virtual ~TypeItem()
		{
			if( fOwnData ) {
				free(fData);
				fData = 0;
				fSize = 0;
			}
			SetObject(0);
		}
		
		int32 ID() const					{ return fID; }
		const char* Name() const			{ return fName.String(); }
		const void* Data() const			{ return fData; }
		size_t Size() const					{ return fSize; }
		
		void SetObject(BResourceSet::TypeObject* object)
		{
			if( fObject ) fObject->Delete();
			fObject = object;
		}
		BResourceSet::TypeObject* Object() const	{ return fObject; }
		
		void SetSourceIsFile(bool state)	{ fSourceIsFile = state; }
		bool SourceIsFile() const			{ return fSourceIsFile; }
		
	private:
		int32 fID;
		BString fName;
		void* fData;
		size_t fSize;
		BResourceSet::TypeObject* fObject;
		bool fOwnData;
		bool fSourceIsFile;
	};
	
	static bool free_type_item_func(void* item)
	{
		delete reinterpret_cast<TypeItem*>(item);
		return false;
	}

	class TypeList {
	public:
		TypeList(type_code type)
			: fType(type)
		{
		}
		
		virtual ~TypeList()
		{
			fItems.DoForEach(free_type_item_func);
			fItems.MakeEmpty();
		}
		
		type_code Type() const		{ return fType; }
		
		TypeItem* FindItemByID(int32 id)
		{
			for( int32 i=0; i<fItems.CountItems(); i++ ) {
				TypeItem* it = (TypeItem*)fItems.ItemAt(i);
				if( it->ID() == id ) return it;
			}
			return 0;
		}
		
		TypeItem* FindItemByName(const char* name)
		{
			for( int32 i=0; i<fItems.CountItems(); i++ ) {
				TypeItem* it = (TypeItem*)fItems.ItemAt(i);
				if( strcmp(it->Name(), name) == 0 ) return it;
			}
			return 0;
		}
		
		void AddItem(TypeItem* item)
		{
			fItems.AddItem(item);
		}
		
	private:
		type_code fType;
		BList fItems;
	};

}	// namespace BResourcePrivate
}	// namespace BExperimental

using namespace BExperimental::BResourcePrivate;

// ----------------------------- BStringMap -----------------------------

BStringMap::BStringMap()
{
}

BStringMap::~BStringMap()
{
}

const char* BStringMap::FindString(const char* name)
{
	return getenv(name);
}

// ----------------------------- TypeObject -----------------------------

BResourceSet::TypeObject::TypeObject()
	: fDeleteOK(false)
{
}
BResourceSet::TypeObject::~TypeObject()
{
	if( !fDeleteOK ) debugger("deleting object owned by BResourceSet");
}
void BResourceSet::TypeObject::Delete()
{
	fDeleteOK = true;
	delete this;
}
		
BResourceSet::BitmapObject::BitmapObject(BRect bounds,
										uint32 flags,
										color_space depth,
										int32 bytesPerRow,
										screen_id screenID)
	: BBitmap(bounds, flags, depth, bytesPerRow, screenID)
{
}

BResourceSet::BitmapObject::BitmapObject(const BBitmap* source,
										bool accepts_views,
										bool need_contiguous)
	: BBitmap(source, accepts_views, need_contiguous)
{
}

BResourceSet::BitmapObject::BitmapObject(BMessage *data)
	: BBitmap(data)
{
}

BResourceSet::BitmapObject::~BitmapObject()
{
}

BResourceSet::CursorObject::CursorObject(const void* data)
	: BCursor(data)
{
}

BResourceSet::CursorObject::CursorObject(BMessage *data)
	: BCursor(data)
{
}

BResourceSet::CursorObject::~CursorObject()
{
}

BResourceSet::MessageObject::MessageObject()
{
}

BResourceSet::MessageObject::~MessageObject()
{
}

// ----------------------------- BResourceSet -----------------------------

BResourceSet::BResourceSet()
{
}

#if USE_RESOURCES
static bool free_resources_func(void* item)
{
	delete reinterpret_cast<BResources*>(item);
	return false;
}
#endif

static bool free_path_func(void* item)
{
	delete reinterpret_cast<BPath*>(item);
	return false;
}

static bool free_type_func(void* item)
{
	delete reinterpret_cast<TypeList*>(item);
	return false;
}

BResourceSet::~BResourceSet()
{
	BAutolock l(fLock);
#if USE_RESOURCES
	fResources.DoForEach(free_resources_func);
	fResources.MakeEmpty();
#endif
	fDirectories.DoForEach(free_path_func);
	fDirectories.MakeEmpty();
	fTypes.DoForEach(free_type_func);
	fTypes.MakeEmpty();
}

status_t BResourceSet::AddResources(BResources* resources, bool at_front)
{
#if USE_RESOURCES
	if( !resources ) return B_BAD_VALUE;
	
	BAutolock l(fLock);
	status_t err = (at_front ? fResources.AddItem(resources, 0)
							 : fResources.AddItem(resources))
				 ? B_OK : B_ERROR;
	if( err != B_OK ) delete resources;
	return err;
#else
	(void)resources;
	(void)at_front;
	return B_ERROR;
#endif
}

// Return the image_id corresponding to the given address, in either
// its text or data section.
static image_id find_image(const void *image_addr)
{
	image_info info; 
	int32 cookie = 0; 
	while (get_next_image_info(0, &cookie, &info) == B_OK) 
		if ((info.text <= image_addr && (((uint8 *)info.text)+info.text_size) > image_addr)
			||(info.data <= image_addr && (((uint8 *)info.data)+info.data_size) > image_addr)) 
			// Found the image.
			return info.id;
	
	return -1;
}

status_t BResourceSet::AddResources(const void* image_addr, bool at_front)
{
#if USE_RESOURCES
	// Lookup the image for this address.
	image_id image = find_image(image_addr);
	if( image < B_OK ) return image;
	
	// Get information about the returned image.
	image_info info;
	status_t err = get_image_info(image, &info);
	if( err != B_OK ) return err;
	
	// Open the image's file.
	BFile file;
	err = file.SetTo(&info.name[0], B_READ_ONLY);
	if( err != B_OK ) return err;
	
	// Create a new resource object on that file.
	BResources* res = new BResources;
	if( !res ) return B_NO_MEMORY;
	
	err = res->SetTo(&file);
	if( err != B_OK ) {
		delete res;
		return err;
	}
	
	// Add it to the set.
	err = AddResources(res, at_front);
	if( err != B_OK ) delete res;
	
	return err;
#else
	(void)image_addr;
	return B_ERROR;
#endif
}

status_t BResourceSet::AddDirectory(const char* full_path, bool at_front)
{
	if( !full_path ) return B_BAD_VALUE;
	BPath* p = new BPath(full_path);
	status_t err = p->InitCheck();
	if( err != B_OK ) {
		delete p;
		return err;
	}
	
	BAutolock l(fLock);
	err = (at_front ? fDirectories.AddItem(p, 0)
					: fDirectories.AddItem(p))
		? B_OK : B_ERROR;
	if( err != B_OK ) delete p;
	return err;
}

status_t BResourceSet::AddEnvDirectory(const char* in,
									   const char* default_value,
									   bool at_front,
									   BStringMap* variables)
{
	BStringMap def_variables;
	if( !variables ) variables = &def_variables;
	
	BString buf;
	status_t err = expand_string(&buf, in, variables);
	
	if( err != B_OK ) {
		if( default_value ) return AddDirectory(default_value, at_front);
		return err;
	}
	
	return AddDirectory(buf.String(), at_front);
}

status_t BResourceSet::expand_string(BString* out, const char* in,
									 BStringMap* variables)
{
	PRINT(("Expanding string: %s\n", in));
	
	const char* start = in;
	while( *in ) {
		if( *in == '$' ) {
			if( start < in ) out->Append(start, (int32)(in-start));
			
			in++;
			char variableName[1024];
			size_t i = 0;
			if( *in == '{' ) {
				in++;
				while( *in && *in != '}' && i<sizeof(variableName)-1 )
					variableName[i++] = *in++;
				if( *in ) in++;
			} else {
				while( isalnum(*in) || *in == '_' && i<sizeof(variableName)-1 )
					variableName[i++] = *in++;
			}
			
			start = in;
			
			variableName[i] = '\0';
			
			const char *val = variables
							? variables->FindString(variableName)
							: 0;
			
			if( !val ) {
				PRINT(("Error: env var %s not found.\n", &variableName[0]));
				return B_NAME_NOT_FOUND;
			}
			
			status_t err = expand_string(out, val, variables);
			if( err != B_OK ) return err;
			
		} else if( *in == '\\' ) {
			if( start < in ) out->Append(start, (int32)(in-start));
			in++;
			start = in;
			in++;
			
		} else
			in++;
	}

	if( start < in ) out->Append(start, (int32)(in-start));
	
	PRINT(("-> Expanded %s to %s\n", in, out->String()));
	
	return B_OK;
}

const void* BResourceSet::FindResource(type_code type, int32 id,
									   size_t* out_size)
{
	TypeItem* item = find_item_id(type, id);
	
	if( out_size ) *out_size = item ? item->Size() : 0;
	return item ? item->Data() : 0;
}

const void* BResourceSet::FindResource(type_code type, const char* name,
									   size_t* out_size)
{
	TypeItem* item = find_item_name(type, name);
	
	if( out_size ) *out_size = item ? item->Size() : 0;
	return item ? item->Data() : 0;
}

// ------------

const BBitmap* BResourceSet::FindBitmap(type_code type, int32 id)
{
	return return_bitmap_item(type, find_item_id(type, id));
}

const BBitmap* BResourceSet::FindBitmap(type_code type, const char* name)
{
	return return_bitmap_item(type, find_item_name(type, name));
}

const BBitmap* BResourceSet::FindBitmap(int32 id)
{
	return return_bitmap_item(B_BITMAP_TYPE, find_item_id(B_BITMAP_TYPE, id));
}

const BBitmap* BResourceSet::FindBitmap(const char* name)
{
	return return_bitmap_item(B_BITMAP_TYPE, find_item_name(B_BITMAP_TYPE, name));
}

// ------------

#if HAVE_CURSORS
const BCursor* BResourceSet::FindCursor(type_code type, int32 id)
{
	return return_cursor_item(find_item_id(type, id));
}

const BCursor* BResourceSet::FindCursor(type_code type, const char* name)
{
	return return_cursor_item(find_item_name(type, name));
}

const BCursor* BResourceSet::FindCursor(int32 id)
{
	return return_cursor_item(find_item_id(B_CURSOR_TYPE, id));
}

const BCursor* BResourceSet::FindCursor(const char* name)
{
	return return_cursor_item(find_item_name(B_CURSOR_TYPE, name));
}
#endif

// ------------

const BMessage* BResourceSet::FindMessage(type_code type, int32 id)
{
	return return_message_item(find_item_id(type, id));
}

const BMessage* BResourceSet::FindMessage(type_code type, const char* name)
{
	return return_message_item(find_item_name(type, name));
}
	
const BMessage* BResourceSet::FindMessage(int32 id)
{
	return return_message_item(find_item_id(B_MESSAGE_TYPE, id));
}

const BMessage* BResourceSet::FindMessage(const char* name)
{
	return return_message_item(find_item_name(B_MESSAGE_TYPE, name));
}
	
// ------------

TypeList* BResourceSet::find_type_list(type_code type)
{
	BAutolock l(fLock);
	
	for( int32 i=0; i<fTypes.CountItems(); i++ ) {
		TypeList* list = (TypeList*)fTypes.ItemAt(i);
		if( list && list->Type() == type ) return list;
	}
	
	return 0;
}

TypeItem* BResourceSet::find_item_id(type_code type, int32 id)
{
	TypeList* list = find_type_list(type);
	TypeItem* item = 0;
	
	if( list ) {
		BAutolock _l(fLock);
		item = list->FindItemByID(id);
	}
	
	if( !item ) {
		item = load_resource(type, id, 0, &list);
	}
	
	return item;
}

TypeItem* BResourceSet::find_item_name(type_code type, const char* name)
{
	TypeList* list = find_type_list(type);
	TypeItem* item = 0;
	
	if( list ) {
		BAutolock _l(fLock);
		item = list->FindItemByName(name);
	}
	
	if( !item ) {
		item = load_resource(type, -1, name, &list);
	}
	
	return item;
}

TypeItem* BResourceSet::load_resource(type_code type, int32 id,
									  const char* name,
									  TypeList** inout_list)
{
	TypeItem* item = 0;
	
	if( name ) {
		BEntry entry;
		
		// If a named resource, first look in directories.  Keep
		// the object locked while looking, so nobody tramples us.
		fLock.Lock();
		for( int32 i=0; item == 0 && i<fDirectories.CountItems(); i++ ) {
			BPath* dir = (BPath*)fDirectories.ItemAt(i);
			if( dir ) {
				fLock.Unlock();
				// For each directory, try to open the named file.
				// We unlock the resource set at this point so that
				// other threads are not blocked while we do a
				// possibly length operation.
				BPath path(dir->Path(), name);
				if( entry.SetTo(path.Path(), true) == B_OK ) {
					BFile file(&entry, B_READ_ONLY);
					if( file.InitCheck() == B_OK ) {
						item = new TypeItem(id, name, &file);
						item->SetSourceIsFile(true);
					}
				}
				fLock.Lock();
			}
		}
		fLock.Unlock();
	}
	
#if USE_RESOURCES
	if( !item ) {
		// Look through resource objects for data.  Keep
		// the object locked while looking, so nobody tramples us.
		fLock.Lock();
		for( int32 i=0; item == 0 && i<fResources.CountItems(); i++ ) {
			BResources* r = (BResources*)fResources.ItemAt(i);
			if( r ) {
				// Note we DON'T unlock here, because BResources is
				// not thread safe (though it -really- should be).
				const void* data = 0;
				size_t size = 0;
				if( id >= 0 ) {
					data = r->LoadResource(type, id, &size);
				} else if( name != 0 ) {
					data = r->LoadResource(type, name, &size);
				}
				if( data && size ) {
					item = new TypeItem(id, name, data, size);
					item->SetSourceIsFile(false);
				}
			}
		}
		fLock.Unlock();
	}
#endif

	if( item ) {
		BAutolock l(fLock);
		
		TypeList* list = inout_list ? *inout_list : 0;
		if( !list ) {
			// Don't currently have a list for this type -- check if there is
			// already one.
			list = find_type_list(type);
		}
		
		if( !list ) {
			// Need to make a new list for this type.
			list = new TypeList(type);
			fTypes.AddItem(list);
			list->AddItem(item);
		
		} else {
			// Check to make sure someone else hasn't already added this
			// item.
			TypeItem* existing = name ? list->FindItemByName(name)
									  : list->FindItemByID(id);
			if( existing ) {
				// This one has already been added.  Delete what we went
				// to all the work to make, and return the other.  *sniff*
				delete item;
				item = existing;
			} else {
				list->AddItem(item);
			}
		}
		
		if( inout_list ) *inout_list = list;
	}
	
	return item;
}

BBitmap* BResourceSet::return_bitmap_item(type_code type, TypeItem* from)
{
	(void)type;
	if( !from ) return 0;
	
	TypeObject* obj = from->Object();
	BitmapObject* bm = dynamic_cast<BitmapObject*>(obj);
	if( bm ) return bm;
	
	// Can't change an existing object.
	if( obj ) return 0;
	
	// Note that we are creating the bitmap without the resource set
	// being locked, since it is a potentially expensive operations.
	// We thus need to deal with the situation where someone else
	// created this at at the same time, and got it added before us.
	bm = GenerateBitmap(from->Data(), from->Size());
	
	if( bm ) {
		BAutolock l(fLock);
		if( from->Object() != 0 ) {
			// Whoops!  Someone snuck in under us.
			bm->Delete();
			bm = dynamic_cast<BitmapObject*>(from->Object());
		} else {
			from->SetObject(bm);
		}
	}
	
	return bm;
}

enum {
	PNG_CHECK_BYTES = 8
};

static void read_png_data(png_structp png_ptr,
						  png_bytep data,
						  png_size_t length)
{
	BDataIO* io = (BDataIO*)png_get_io_ptr(png_ptr);
	ssize_t amount = io->Read(data, length);
	if( amount < B_OK ) png_error(png_ptr, "Read Error");
}

BResourceSet::BitmapObject* BResourceSet::read_png_image(BDataIO* stream)
{
	png_byte buf[PNG_CHECK_BYTES];
	
	// Make sure there is a header
	if( stream->Read(buf, PNG_CHECK_BYTES) != PNG_CHECK_BYTES ) return 0;
	
	// Check the header
	if( png_sig_cmp(buf, 0, PNG_CHECK_BYTES) != 0 ) return 0;
	
	png_structp png_ptr;
	png_infop info_ptr;
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return 0;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp) NULL);
		return 0;
	}
	
//P	if (setjmp(png_ptr->jmpbuf)) {
//P		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
//P		return 0;
//P	}
	
	png_set_read_fn(png_ptr, stream, read_png_data);
	png_set_sig_bytes(png_ptr, PNG_CHECK_BYTES);
	
	png_read_info(png_ptr, info_ptr);
	
	// Set up color space conversion
	png_byte color_type(png_get_color_type(png_ptr, info_ptr));
	png_byte bit_depth(png_get_bit_depth(png_ptr, info_ptr));

	if (bit_depth <= 8) {
		png_set_expand(png_ptr);
		png_set_packing(png_ptr);
	}

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_ptr);

	if (bit_depth > 8)
		png_set_strip_16(png_ptr);
	
	png_set_bgr(png_ptr);

	if (!(color_type & PNG_COLOR_MASK_COLOR))
		png_set_gray_to_rgb(png_ptr);

	if (!(color_type & PNG_COLOR_MASK_ALPHA))
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
	
	png_read_update_info(png_ptr, info_ptr);
	png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
	png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
	
	// Create bitmap
	BitmapObject* bitmap =
		new BitmapObject(BRect(0, 0, width-1, height-1), 0, B_RGBA32);
	
	png_bytep* rows = new png_bytep[height];
	for( size_t i=0; i<height; i++ ) {
		rows[i] = ((png_bytep)bitmap->Bits()) + i*bitmap->BytesPerRow();
	}
	png_read_image(png_ptr, rows);
	
	png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp) NULL);
	
	return bitmap;
}

BResourceSet::BitmapObject*
BResourceSet::GenerateBitmap(const void* data, size_t size)
{
	// Don't have a bitmap in the item -- we'll try to create one.
	BMemoryIO stream(data, size);
	
	// First, see if it can be read as a png image.
	BitmapObject* bm = read_png_image(&stream);
	
	if( !bm ) {
		// Well, maybe it's an archived bitmap.
		stream.Seek(0, SEEK_SET);
		BMessage archive;
		if( archive.Unflatten(&stream) == B_OK ) {
			bm = new BitmapObject(&archive);
			if( bm && bm->InitCheck() != B_OK ) {
				bm->Delete();
			}
		}
	}
	
	return bm;
}

#if HAVE_CURSORS
BCursor* BResourceSet::return_cursor_item(TypeItem* from)
{
	if( !from ) return 0;
	
	TypeObject* obj = from->Object();
	CursorObject* cr = dynamic_cast<CursorObject*>(obj);
	if( cr ) return cr;
	
	// Can't change an existing object.
	if( obj ) return 0;
	
	// Don't have a cursor in the item -- we'll create one.
	// Note that we are creating the cursor without the resource set
	// being locked, since it is a potentially expensive operations.
	// We thus need to deal with the situation where someone else
	// created this at at the same time, and got it added before us.
	cr = GenerateCursor(from->Data(), from->Size());
	
	if( cr ) {
		BAutolock l(fLock);
		if( from->Object() != 0 ) {
			// Whoops!  Someone snuck in under us.
			cr->Delete();
			cr = dynamic_cast<CursorObject*>(from->Object());
		} else {
			from->SetObject(cr);
		}
	}
	
	return cr;
}

BResourceSet::CursorObject*
BResourceSet::GenerateCursor(const void* data, size_t size)
{
	return new CursorObject(data);
}
#endif

BMessage* BResourceSet::return_message_item(TypeItem* from)
{
	if( !from ) return 0;
	
	TypeObject* obj = from->Object();
	MessageObject* ms = dynamic_cast<MessageObject*>(obj);
	if( ms ) return ms;
	
	// Can't change an existing object.
	if( obj ) return 0;
	
	// Note that we are creating the message without the resource set
	// being locked, since it is a potentially expensive operations.
	// We thus need to deal with the situation where someone else
	// created this at at the same time, and got it added before us.
	ms = GenerateMessage(from->Data(), from->Size());
	
	if( ms ) {
		BAutolock l(fLock);
		if( from->Object() != 0 ) {
			// Whoops!  Someone snuck in under us.
			ms->Delete();
			ms = dynamic_cast<MessageObject*>(from->Object());
		} else {
			from->SetObject(ms);
		}
	}
	
	return ms;
}

BResourceSet::MessageObject*
BResourceSet::GenerateMessage(const void* data, size_t size)
{
	MessageObject* ms = new MessageObject();
	if (ms->Unflatten((const char*)data) != B_OK) {
		ms->Delete();
		ms = NULL;
	}
	return ms;
}
