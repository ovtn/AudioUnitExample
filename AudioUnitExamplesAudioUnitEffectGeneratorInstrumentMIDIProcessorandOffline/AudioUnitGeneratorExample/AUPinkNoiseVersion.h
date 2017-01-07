/*
Copyright (C) 2016 Apple Inc. All Rights Reserved.
See LICENSE.txt for this sample’s licensing information

Abstract:
Pink Noise AU
*/

#ifndef __AUPinkNoiseVersion_h__
#define __AUPinkNoiseVersion_h__


#ifdef DEBUG
	#define kAUPinkNoiseVersion 0xFFFFFFFF
#else
	#define kAUPinkNoiseVersion 0x00010000	
#endif

//~~~~~~~~~~~~~~  Change!!! ~~~~~~~~~~~~~~~~~~~~~//
#define AUPinkNoise_COMP_SUBTYPE	'pink'
#define AUPinkNoise_COMP_MANF		'Demo'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#endif

