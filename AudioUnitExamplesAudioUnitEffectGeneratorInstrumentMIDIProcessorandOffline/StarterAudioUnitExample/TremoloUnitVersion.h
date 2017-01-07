/*
Copyright (C) 2016 Apple Inc. All Rights Reserved.
See LICENSE.txt for this sample’s licensing information

Abstract:
Tremolo Effect AU
*/

#ifndef __TremoloUnitVersion_h__
#define __TremoloUnitVersion_h__


#ifdef DEBUG
	#define kTremoloUnitVersion 0xFFFFFFFF
#else
	#define kTremoloUnitVersion 0x00010000	
#endif

// customized for each audio unit
#define TremoloUnit_COMP_SUBTYPE		'tmlo'
#define TremoloUnit_COMP_MANF			'Aaud'

#endif

