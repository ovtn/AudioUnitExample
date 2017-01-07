/*
Copyright (C) 2016 Apple Inc. All Rights Reserved.
See LICENSE.txt for this sample’s licensing information

Abstract:
Instrument AU
*/

/*
	This is a subclass of SinSynth that demonstrates how to use the midi output properties kAudioUnitProperty_MIDIOutputCallbackInfo,
    and kAudioUnitProperty_MIDIOutputCallback defined in AudioUnitProperties.h.
    Using these properties, the SinSynthWithMidi simply passes through the midi data it receives. Use of these properties requires host support.
	
	To build a version of the SinSynth with this functionality, activate the "SinSynth with MIDI Output" target in Xcode.
*/

#include "SinSynth.h"
#include <CoreMIDI/CoreMIDI.h>
#include <vector>

typedef struct MIDIMessageInfoStruct {
	UInt8	status;
	UInt8	channel;
	UInt8	data1;
	UInt8	data2;
	UInt32	startFrame;
} MIDIMessageInfoStruct;


class MIDIOutputCallbackHelper 
{
	enum  { kSizeofMIDIBuffer = 512 };
	
public:
							MIDIOutputCallbackHelper() 
							{
								mMIDIMessageList.reserve (64); 
								mMIDICallbackStruct.midiOutputCallback = NULL;
								mMIDIBuffer = new Byte[kSizeofMIDIBuffer];
							}

							~MIDIOutputCallbackHelper() 
							{
								delete [] mMIDIBuffer;
							}
							
	void					SetCallbackInfo (AUMIDIOutputCallback & callback, void *userData) 
							{
								mMIDICallbackStruct.midiOutputCallback = callback; 
								mMIDICallbackStruct.userData = userData;
							}
	
	void					AddMIDIEvent (UInt8		status,
										UInt8		channel,
										UInt8		data1,
										UInt8		data2, 
										UInt32		inStartFrame );
							
	void					FireAtTimeStamp(const AudioTimeStamp &inTimeStamp);

	
private:
	MIDIPacketList		  * PacketList() 
							{
								return (MIDIPacketList *)mMIDIBuffer; 
							}

	
	Byte *						mMIDIBuffer;
	
	AUMIDIOutputCallbackStruct	mMIDICallbackStruct;

	typedef std::vector<MIDIMessageInfoStruct> MIDIMessageList;
	MIDIMessageList				mMIDIMessageList;
};

class SinSynthWithMidi : public SinSynth {
public:
								SinSynthWithMidi(AudioUnit inComponentInstance);
	virtual						~SinSynthWithMidi();
	
	virtual OSStatus			GetPropertyInfo(		AudioUnitPropertyID				inID,
														AudioUnitScope					inScope,
														AudioUnitElement				inElement,
														UInt32 &						outDataSize,
														Boolean &						outWritable);
	
	virtual OSStatus			GetProperty(			AudioUnitPropertyID				inID,
														AudioUnitScope					inScope,
														AudioUnitElement				inElement,
														void *							outData);
														
	virtual OSStatus			SetProperty(			AudioUnitPropertyID 			inID,
														AudioUnitScope 					inScope,
														AudioUnitElement 				inElement,
														const void *					inData,
														UInt32 							inDataSize);
														
			OSStatus			HandleMidiEvent(		UInt8							status, 
														UInt8							channel, 
														UInt8							data1, 
														UInt8							data2, 
														UInt32							inStartFrame);
	
			OSStatus			Render(					AudioUnitRenderActionFlags &	ioActionFlags,
														const AudioTimeStamp &			inTimeStamp,
														UInt32							inNumberFrames);

private:
	MIDIOutputCallbackHelper	mCallbackHelper;
	TestNote					mTestNotes[kNumNotes];
};

#pragma mark MIDIOutputCallbackHelper Methods

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void MIDIOutputCallbackHelper::AddMIDIEvent(UInt8	status, 
										UInt8		channel,
										UInt8		data1, 
										UInt8		data2, 
										UInt32		inStartFrame) 
{
	MIDIMessageInfoStruct info = {status, channel, data1, data2, inStartFrame};	
	mMIDIMessageList.push_back(info);
}

void MIDIOutputCallbackHelper::FireAtTimeStamp(const AudioTimeStamp &inTimeStamp) 
{
	if (!mMIDIMessageList.empty())
	{
		if (mMIDICallbackStruct.midiOutputCallback) 
		{
			// synthesize the packet list and call the MIDIOutputCallback
			// iterate through the vector and get each item
			MIDIPacketList *pktlist = PacketList();

			MIDIPacket *pkt = MIDIPacketListInit(pktlist);
			
			for (MIDIMessageList::iterator iter = mMIDIMessageList.begin(); iter != mMIDIMessageList.end(); iter++) 
			{
				const MIDIMessageInfoStruct & item = *iter;
								
				Byte midiStatusByte = item.status + item.channel;
				const Byte data[3] = { midiStatusByte, item.data1, item.data2 };
				UInt32 midiDataCount = ((item.status == 0xC || item.status == 0xD) ? 2 : 3);
				pkt = MIDIPacketListAdd (pktlist, 
											kSizeofMIDIBuffer, 
											pkt, 
											item.startFrame, 
											midiDataCount, 
											data);
				if (!pkt)
				{
						// send what we have and then clear the buffer and then go through this again
					// issue the callback with what we got
					OSStatus result = (*mMIDICallbackStruct.midiOutputCallback) (mMIDICallbackStruct.userData, &inTimeStamp, 0, pktlist);
					if (result != noErr)
						printf("error calling output callback: %d", (int) result);
					
					// clear stuff we've already processed, and fire again
					mMIDIMessageList.erase (mMIDIMessageList.begin(), iter);
					FireAtTimeStamp(inTimeStamp);
					return;
				}
			}
			
			// fire callback
			OSStatus result = (*mMIDICallbackStruct.midiOutputCallback) (mMIDICallbackStruct.userData, &inTimeStamp, 0, pktlist);
			if (result != noErr)
				printf("error calling output callback: %d", (int) result);
		}
		mMIDIMessageList.clear();
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark SinSynthWithMidi Methods

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, SinSynthWithMidi)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	SinSynthWithMidi::SinSynthWithMidi
//
// This synth has No inputs, One output
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SinSynthWithMidi::SinSynthWithMidi(AudioUnit inComponentInstance)
	: SinSynth(inComponentInstance)
{
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	SinSynthWithMidi::~SinSynthWithMidi
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SinSynthWithMidi::~SinSynthWithMidi()
{}

OSStatus			SinSynthWithMidi::GetPropertyInfo(		AudioUnitPropertyID				inID,
													AudioUnitScope					inScope,
													AudioUnitElement				inElement,
													UInt32 &						outDataSize,
													Boolean &						outWritable)
{
	if (inScope == kAudioUnitScope_Global) {
		if (inID == kAudioUnitProperty_MIDIOutputCallbackInfo) {
			outDataSize = sizeof(CFArrayRef);
			outWritable = false;
			return noErr;
		} else if (inID == kAudioUnitProperty_MIDIOutputCallback) {
			outDataSize = sizeof(AUMIDIOutputCallbackStruct);
			outWritable = true;
			return noErr;
		}
	}
	return SinSynth::GetPropertyInfo (inID, inScope, inElement, outDataSize, outWritable);
}

OSStatus			SinSynthWithMidi::GetProperty(	AudioUnitPropertyID		inID,
											AudioUnitScope			inScope,
											AudioUnitElement		inElement,
											void *					outData)
{
	if (inScope == kAudioUnitScope_Global) 
	{
		if (inID == kAudioUnitProperty_MIDIOutputCallbackInfo) {
			CFStringRef strs[1];
			strs[0] = CFSTR("MIDI Callback");
			
			CFArrayRef callbackArray = CFArrayCreate(NULL, (const void **)strs, 1, &kCFTypeArrayCallBacks);
			*(CFArrayRef *)outData = callbackArray;
			return noErr;
		}
	}
	return SinSynth::GetProperty (inID, inScope, inElement, outData);
}

OSStatus			SinSynthWithMidi::SetProperty(	AudioUnitPropertyID 			inID,
													AudioUnitScope 					inScope,
													AudioUnitElement 				inElement,
													const void *					inData,
													UInt32 							inDataSize)
{
	if (inScope == kAudioUnitScope_Global) 
	{
		if (inID == kAudioUnitProperty_MIDIOutputCallback) {
			if (inDataSize < sizeof(AUMIDIOutputCallbackStruct)) return kAudioUnitErr_InvalidPropertyValue;
			
			AUMIDIOutputCallbackStruct *callbackStruct = (AUMIDIOutputCallbackStruct *)inData;
			mCallbackHelper.SetCallbackInfo(callbackStruct->midiOutputCallback, callbackStruct->userData);
			return noErr;
		}
	}
	return SinSynth::SetProperty(inID, inScope, inElement, inData, inDataSize);
}

OSStatus 	SinSynthWithMidi::HandleMidiEvent(UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2, UInt32 inStartFrame) 
{
	// snag the midi event and then store it in a vector	
	mCallbackHelper.AddMIDIEvent(status, channel, data1, data2, inStartFrame);
	
	return AUMIDIBase::HandleMidiEvent(status, channel, data1, data2, inStartFrame);
}

OSStatus	SinSynthWithMidi::Render(   AudioUnitRenderActionFlags &		ioActionFlags,
											const AudioTimeStamp &			inTimeStamp,
											UInt32							inNumberFrames) 
{
	OSStatus result = AUInstrumentBase::Render(ioActionFlags, inTimeStamp, inNumberFrames);
	if (result == noErr) {
		mCallbackHelper.FireAtTimeStamp(inTimeStamp);
	} 
	return result;
}
