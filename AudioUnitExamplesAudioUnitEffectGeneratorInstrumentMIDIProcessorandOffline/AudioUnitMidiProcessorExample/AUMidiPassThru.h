/*
Copyright (C) 2016 Apple Inc. All Rights Reserved.
See LICENSE.txt for this sample’s licensing information

Abstract:
MIDI Processor AU
*/

#ifndef __AUMidiPassThru_h__
#define __AUMidiPassThru_h__

#include "AUMidiPassThruVersion.h"
#include <CoreMIDI/CoreMIDI.h>
#include "AUMIDIEffectBase.h"
#include "LockFreeFIFO.h"


#pragma mark - AUMidiPassThru
class AUMidiPassThru : public AUMIDIEffectBase
{
public:
	AUMidiPassThru(AudioUnit component);
	virtual ~AUMidiPassThru();

	virtual OSStatus GetPropertyInfo(AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, UInt32& outDataSize, Boolean& outWritable );

	virtual OSStatus GetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void* outData);
  
    virtual OSStatus SetProperty(	AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void* inData, UInt32 inDataSize);

 	virtual	bool SupportsTail() { return false; }

	virtual OSStatus Version() { return kAUMidiPassThruVersion; }

    virtual OSStatus HandleMidiEvent(UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2, UInt32 inOffsetSampleFrame);
  
    virtual OSStatus Render(AudioUnitRenderActionFlags &ioActionFlags, const AudioTimeStamp& inTimeStamp, UInt32 nFrames);

private:
    AUMIDIOutputCallbackStruct mMIDIOutCB;
  
    LockFreeFIFO<MIDIPacket> mOutputPacketFIFO;
  
};

#endif
