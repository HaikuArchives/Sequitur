/* GlChainMacro.h
 * Copyright (c)2004 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
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
 * 2004.04.24				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLCHAINMACRO_H
#define GLPUBLIC_GLCHAINMACRO_H

#include <ArpCore/String16.h>
#include <be/support/SupportDefs.h>
class GlChain;
class _GlChainMacroData;

/***************************************************************************
 * GL-CHAIN-MACRO
 * A utility that stores a high-level description of a chain.
 ***************************************************************************/
class GlChainMacro
{
public:
	GlChainMacro();
	GlChainMacro(const GlChainMacro& o);
	~GlChainMacro();

	GlChainMacro&		operator=(const GlChainMacro& o);
	GlChainMacro*		Clone() const;
	status_t			Status() const;
	
	/* Chains are created by Add()ing a description of
	 * a node, then calling Set() for any properties you
	 * want specified.  Set() always operates on the last
	 * Add()ed node.  The first variant of Add() assumes
	 * the creator is "arp" -- it's just a small shortcut.
	 */
	GlChainMacro&		Add(int32 key);
	GlChainMacro&		Add(const BString16& creator, int32 key);
	/* Multiple calls with the same key will add multiple
	 * params.  Some clients want this (for example, number
	 * input that can be several keys).  For the ones that
	 * don't, they need to be aware enough not to do it.
	 */
	GlChainMacro&		Set(int32 key, int32 value);
	GlChainMacro&		SetF(int32 key, float value);
	GlChainMacro&		Set(int32 key, const BString16& value);
	/* Create a new subchain for the current chain and answer it.
	 */
	GlChainMacro&		Sub(uint32 index);

	void				MakeEmpty();

	/* UTILITIES
	   --------- */
	bool				Matches(const GlChain* c) const;
	/* Add all my info as nodes and chains to the chain.
	 */
	status_t			Install(GlChain* c) const;
	
private:
	_GlChainMacroData*	mData;
	int32				mCur;
	status_t			mStatus;
	
public:
	void				Print(uint32 tabs = 0) const;
};

#endif
