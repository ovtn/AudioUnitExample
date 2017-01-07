/*
Copyright (C) 2016 Apple Inc. All Rights Reserved.
See LICENSE.txt for this sample’s licensing information

Abstract:
MIDI Processor AU
*/

#include "AUMidiPassThru.h"

static const int kMIDIPacketListSize = 2048;

AUDIOCOMPONENT_ENTRY(AUMIDIEffectFactory, AUMidiPassThru)

#pragma mark AUMidiPassThru

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	AUMidiPassThru::SetProperty
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUMidiPassThru::AUMidiPassThru(AudioUnit component) : AUMIDIEffectBase(component), mOutputPacketFIFO(LockFreeFIFO<MIDIPacket>(32))
{
	CreateElements();
    
    mMIDIOutCB.midiOutputCallback = nullptr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	AUMidiPassThru::SetProperty
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUMidiPassThru::~AUMidiPassThru()
{
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	AUMidiPassThru::SetProperty
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus AUMidiPassThru::GetPropertyInfo(AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, UInt32 & outDataSize, Boolean & outWritable)
{
    if (inScope == kAudioUnitScope_Global)
    {
        switch( inID )
        {
            case kAudioUnitProperty_MIDIOutputCallbackInfo:
                outWritable = false;
                outDataSize = sizeof(CFArrayRef);
                return noErr;
                
            case kAudioUnitProperty_MIDIOutputCallback:
                outWritable = true;
                outDataSize = sizeof(AUMIDIOutputCallbackStruct);
                return noErr;
        }
	}
	return AUMIDIEffectBase::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	AUMidiPassThru::SetProperty
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus AUMidiPassThru::GetProperty( AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void* outData )
{
	if (inScope == kAudioUnitScope_Global) 
	{
        switch (inID)
		{
            case kAudioUnitProperty_MIDIOutputCallbackInfo:
                CFStringRef string = CFSTR("midiOut");
                CFArrayRef array = CFArrayCreate(kCFAllocatorDefault, (const void**)&string, 1, nullptr);
                CFRelease(string);
                *((CFArrayRef*)outData) = array;
                return noErr;
		}
	}
	return AUMIDIEffectBase::GetProperty(inID, inScope, inElement, outData);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	AUMidiPassThru::SetProperty
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus AUMidiPassThru::SetProperty(	AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void* inData, UInt32 inDataSize)
{
    if (inScope == kAudioUnitScope_Global)
    {
        switch (inID)
        {
            case kAudioUnitProperty_MIDIOutputCallback:
            mMIDIOutCB = *((AUMIDIOutputCallbackStruct*)inData);
            return noErr;
        }
    }
	return AUMIDIEffectBase::SetProperty(inID, inScope, inElement, inData, inDataSize);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	AUMidiPassThru::HandleMidiEvent
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus AUMidiPassThru::HandleMidiEvent(UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2, UInt32 inOffsetSampleFrame)
{
    if (!IsInitialized()) return kAudioUnitErr_Uninitialized;
  
    MIDIPacket* packet = mOutputPacketFIFO.WriteItem();
    mOutputPacketFIFO.AdvanceWritePtr();
    
    if (packet == NULL)
        return kAudioUnitErr_FailedInitialization;
    
    memset(packet->data, 0, sizeof(Byte)*256);
    packet->length = 3;
    packet->data[0] = status | channel;
    packet->data[1] = data1;
    packet->data[2] = data2;
    packet->timeStamp = inOffsetSampleFrame;
    
	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	AUMidiPassThru::Render
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus AUMidiPassThru::Render (AudioUnitRenderActionFlags &ioActionFlags, const AudioTimeStamp& inTimeStamp, UInt32 nFrames)
{
    Byte listBuffer[kMIDIPacketListSize];
    MIDIPacketList* packetList = (MIDIPacketList*)listBuffer;
    MIDIPacket* packetListIterator = MIDIPacketListInit(packetList);
  
    MIDIPacket* packet = mOutputPacketFIFO.ReadItem();
    while (packet != NULL)
    {
        //----------------------------------------------------------------------//
        // This is where the midi packets get processed
        //
        //----------------------------------------------------------------------//
        
        if (packetListIterator == NULL) return noErr;
        packetListIterator = MIDIPacketListAdd(packetList, kMIDIPacketListSize, packetListIterator, packet->timeStamp, packet->length, packet->data);
        mOutputPacketFIFO.AdvanceReadPtr();
        packet = mOutputPacketFIFO.ReadItem();
    }
    
    if (mMIDIOutCB.midiOutputCallback != NULL && packetList->numPackets > 0)
    {
        mMIDIOutCB.midiOutputCallback(mMIDIOutCB.userData, &inTimeStamp, 0, packetList);
    }
      
    return noErr;
}



