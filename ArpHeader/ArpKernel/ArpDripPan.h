/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpDripPan.h
 *
 * A class for catching memory leaks caused by exceptions.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * This is still under development -- do not use.
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
 * 11/29/1998:
 *	• Created this file.
 *
 */

#ifndef ARPKERNEL_ARPDRIPPAN_H
#define ARPKERNEL_ARPDRIPPAN_H

template<class T>
class ArpDripPan {
public:
	ArpDripPan()	{ }
	~ArpDripPan()	{ TossAll(); }
	
	void Catch(T* obj) {
		if( obj ) mLeaks.AddItem( (void*)obj );
	}
	void Detach(T* obj) {
		if( obj ) mLeaks.RemoveItem( (void*)obj );
	}
	
	void DetachAll(void) {
		mLeaks.MakeEmpty();
	}
	
	void TossAll(void) {
		int32 num;
		while( (num=mLeaks.CountItems()) > 0 ) {
			void* item = mLeaks.RemoveItem(num-1);
			if( item ) delete ((T*)item);
		}
	}
	
private:
	BList mLeaks;
};

#endif
