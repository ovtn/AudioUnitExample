/*
Copyright (C) 2016 Apple Inc. All Rights Reserved.
See LICENSE.txt for this sample’s licensing information

*/

/*
 This test demonstrates how a midi processing AudioUnit (kAudioUnitType_MIDIProcessor) can be used.
 Note that currently a midi processing AU cannot be a part of an AUGraph. 
 
 In this example, the rendering chain consists of a DLS synth AU and Default Output AU connected
 using an AUGraph. Each midi note is first processed by the midi processor AU and then handed over 
 to the DLS synth AU which renders it to audio.
*/

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#include <deque>

#include "CAXException.h"
#include "AUMidiPassThruVersion.h"

enum
{
    kMidiMessage_NoteOff 			= 0x80,
	kMidiMessage_NoteOn 			= 0x90,
};


struct MIDINote
{
    UInt32      mNoteVal;
    UInt32      mNoteVelocity;
    Float64     mTimeStamp;
};

std::deque<MIDINote>    gMIDINotes;
AudioUnit               gDlsSynthUnit = nullptr;
AudioUnit               gMIDIProcessorUnit = nullptr;
CFRunLoopRef            gRunLoop = nullptr;
AudioBufferList*        gDummyBufferList = nullptr;

// function that creates some midi notes
static void CreateTestMidiNotes(Float64 inSampleRate)
{
    const int numberOfNotesToRender = 12;
    for (int i = 0; i < numberOfNotesToRender; ++i)
    {
        MIDINote midiNote;
        midiNote.mNoteVal = 60 + i;
        midiNote.mNoteVelocity = 127;
        midiNote.mTimeStamp = i * inSampleRate; // render at 1 second intervals
        gMIDINotes.push_back(midiNote);
    }
}


// host function that gets called by the midi processing unit
static OSStatus HostMidiOutputProc (void *							userData,
                                    const AudioTimeStamp *			timeStamp,
                                    UInt32							midiOutNum,
                                    const struct MIDIPacketList *	pktlist)
{
    OSStatus err = noErr;
    
    MIDIPacket *packet = (MIDIPacket *)pktlist->packet;
    for (int i=0; i < pktlist->numPackets; ++i)
    {
        Byte midiStatus = packet->data[0];
        if (midiStatus & kMidiMessage_NoteOn)
        {
            Byte note = packet->data[1] & 0x7F;
            Byte velocity = packet->data[2] & 0x7F;
            err = MusicDeviceMIDIEvent(gDlsSynthUnit, kMidiMessage_NoteOn, note, velocity, static_cast<UInt32>(packet->timeStamp));
            if (err)
            {
                printf("ERROR: HostMidiOutputProc: MusicDeviceMIDIEvent: %d\n",(int)err);
                goto end;
            }
            packet = MIDIPacketNext(packet);
        }
    }
    
    return noErr;
    
end:
    exit(err);
}

// function that gets called by AUGraph before (and after) a render cycle
// this is where midi notes are given to the midi processing unit
static OSStatus GraphRenderNotify (	void *							inRefCon,
                                   AudioUnitRenderActionFlags *     ioActionFlags,
                                   const AudioTimeStamp *			inTimeStamp,
                                   UInt32							inBusNumber,
                                   UInt32							inNumberFrames,
                                   AudioBufferList *				ioData)
{
    OSStatus err = noErr;
    if (*ioActionFlags & kAudioUnitRenderAction_PreRender)
    {
        while (gMIDINotes.size() && gMIDINotes[0].mTimeStamp <= (inTimeStamp->mSampleTime + inNumberFrames - 1))
        {
            MIDINote midiNote = gMIDINotes[0];
            UInt32 offset = midiNote.mTimeStamp - inTimeStamp->mSampleTime;
            err = MusicDeviceMIDIEvent(gMIDIProcessorUnit, kMidiMessage_NoteOn, midiNote.mNoteVal, midiNote.mNoteVelocity, offset);
            if (err)
            {
                printf("ERROR: GraphRenderNotify: MusicDeviceMIDIEvent: %d\n",(int)err);
                goto end;
            }
            gMIDINotes.pop_front();
        }
        
        // call AudioUnitRender on midi processing unit which will in turn call HostMidiOutputProc with the modified midi data
        err = AudioUnitRender(gMIDIProcessorUnit, ioActionFlags, inTimeStamp, 0, inNumberFrames, gDummyBufferList);
        if (err)
        {
            printf("ERROR: GraphRenderNotify: AudioUnitRender: %d\n",(int)err);
            goto end;
        }
        
        if (gMIDINotes.size() == 0)
            CFRunLoopStop(gRunLoop);  //we're out of midi notes so we're done
    }
    return noErr;
    
end:
    exit(err);
}

int main(int argc, const char * argv[])
{
    OSStatus result = noErr;
    
    AUGraph graph = nullptr;
    AUNode dlsSynthNode = NULL, defaultOutputNode = NULL;
    
    AudioComponentDescription dlsSynthDesc{ kAudioUnitType_MusicDevice, kAudioUnitSubType_DLSSynth, kAudioUnitManufacturer_Apple, 0, 0};
    AudioComponentDescription defaultOutputDesc{ kAudioUnitType_Output, kAudioUnitSubType_DefaultOutput, kAudioUnitManufacturer_Apple, 0, 0};
    AudioComponentDescription midiProcessorDesc{ kAudioUnitType_MIDIProcessor, AUMidiPassThru_COMP_SUBTYPE, kAudioUnitManufacturer_Apple, 0, 0};
    
    try {
        AudioComponent midiProcessorComp = AudioComponentFindNext(nullptr, &midiProcessorDesc);
        XThrowIf(midiProcessorComp == nullptr, -1, "Couldn't find AUMidiPassThru. Did you build the AUMidiPassThru target?");
        XThrowIfError(AudioComponentInstanceNew(midiProcessorComp, &gMIDIProcessorUnit), "AudioComponentInstanceNew: AUMidiPassThru");
        XThrowIfError(AudioUnitInitialize(gMIDIProcessorUnit), "AudioUnitInitialize: AUMidiPassThru");
        
        // get the number of midi outputs from the midi processing unit
        CFArrayRef midiOutputs = nullptr;
        UInt32 propSize = sizeof(midiOutputs);
        XThrowIfError(AudioUnitGetProperty(gMIDIProcessorUnit, kAudioUnitProperty_MIDIOutputCallbackInfo, kAudioUnitScope_Global, 0, &midiOutputs, &propSize), "AudioUnitGetProperty: kAudioUnitProperty_MIDIOutputCallbackInfo");
        XThrowIf(midiOutputs == nullptr, -1, "CFArray returned from AudioUnitGetProperty: kAudioUnitProperty_MIDIOutputCallbackInfo is NULL");
        XThrowIf(CFArrayGetCount(midiOutputs) == 0, -1, "Midi output stream count = 0");
        CFRelease(midiOutputs);
        
        // create the rendering graph:   DLS Synth -> Output Unit
        XThrowIfError(NewAUGraph(&graph), "NewAUGraph");
        XThrowIfError(AUGraphAddNode(graph, &dlsSynthDesc, &dlsSynthNode), "AUGraphAddNode: dlsSynth");
        XThrowIfError(AUGraphAddNode(graph, &defaultOutputDesc, &defaultOutputNode), "AUGraphAddNode: defaultOutput");
        XThrowIfError(AUGraphConnectNodeInput(graph, dlsSynthNode, 0, defaultOutputNode, 0), "AUGraphConnectNodeInput");
        XThrowIfError(AUGraphOpen(graph), "AUGraphOpen");
        XThrowIfError(AUGraphNodeInfo(graph, dlsSynthNode, nullptr, &gDlsSynthUnit), "AUGraphNodeInfo");
        
        gRunLoop = CFRunLoopGetCurrent();
        
        // create some midi notes to be played
        const Float64 desiredSampleRate = 44100;
        CreateTestMidiNotes(desiredSampleRate);
        
        // create a dummy bufferlist to satisfy some AUBase checks
        AudioStreamBasicDescription midiProcessorOutputStreamFormat;
        propSize = sizeof(midiProcessorOutputStreamFormat);
        XThrowIfError(AudioUnitGetProperty(gMIDIProcessorUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &midiProcessorOutputStreamFormat, &propSize), "AudioUnitSetProperty: kAudioUnitProperty_StreamFormat");
                
        gDummyBufferList = (AudioBufferList*) malloc(sizeof(AudioBufferList) + (midiProcessorOutputStreamFormat.mChannelsPerFrame - 1) * sizeof(AudioBuffer));
        gDummyBufferList->mNumberBuffers = midiProcessorOutputStreamFormat.mChannelsPerFrame;
        for (int i=0; i<midiProcessorOutputStreamFormat.mChannelsPerFrame; ++i) {
            gDummyBufferList->mBuffers[i].mData = nullptr;
            gDummyBufferList->mBuffers[i].mDataByteSize = 0;
            gDummyBufferList->mBuffers[0].mNumberChannels = 1;
        }
        
        XThrowIfError(AUGraphAddRenderNotify(graph, GraphRenderNotify, nullptr), "AUGraphAddRenderNotify");
        
        // set the midi output callback on the midi processing unit
        AUMIDIOutputCallbackStruct midiOutputCallbackStruct;
        midiOutputCallbackStruct.midiOutputCallback = HostMidiOutputProc;
        midiOutputCallbackStruct.userData = nullptr;
        
        XThrowIfError(AudioUnitSetProperty(gMIDIProcessorUnit, kAudioUnitProperty_MIDIOutputCallback, kAudioUnitScope_Global, 0, &midiOutputCallbackStruct, sizeof(midiOutputCallbackStruct)), "AudioUnitSetProperty: kAudioUnitProperty_MIDIOutputCallback");
        
        // initialize and start the graph
        XThrowIfError(AUGraphInitialize(graph), "AUGraphInitialize");
        XThrowIfError(AUGraphStart(graph), "AUGraphStart");
        
        printf("Starting graph now...\n");
        
        // run until we've run out of MIDI notes - GraphRenderNotify will signal back
        CFRunLoopRun();
        
        // wait for a few more seconds until the last note has finished decaying
        printf("Waiting for 3 secs to allow last note to finish decaying...\n");
        sleep(3);
    }
    
    catch (CAXException &e) {
        printf("ERROR: %s: %d\n\n", e.mOperation, e.mError);
        result = e.mError;
    }
    
    // clean up
    free (gDummyBufferList); gDummyBufferList = nullptr;
    AudioComponentInstanceDispose(gMIDIProcessorUnit);
    DisposeAUGraph(graph);

    return result;
}