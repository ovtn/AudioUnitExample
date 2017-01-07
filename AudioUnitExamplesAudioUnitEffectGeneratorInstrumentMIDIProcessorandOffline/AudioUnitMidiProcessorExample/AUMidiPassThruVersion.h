/*
Copyright (C) 2016 Apple Inc. All Rights Reserved.
See LICENSE.txt for this sample’s licensing information

Abstract:
MIDI Processor AU
*/

#ifndef __AUMidiPassThruVersion_h__
#define __AUMidiPassThruVersion_h__

#ifdef DEBUG
	#define kAUMidiPassThruVersion    0xFFFFFFFF
#else
	#define kAUMidiPassThruVersion    0x00010000
#endif

#define AUMidiPassThru_COMP_TYPE      'aumi'
#define AUMidiPassThru_COMP_SUBTYPE   'aump'
#define AUMidiPassThru_COMP_MANF      'appl'

#endif

