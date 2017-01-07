/*
Copyright (C) 2016 Apple Inc. All Rights Reserved.
See LICENSE.txt for this sample’s licensing information

*/

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	TRandom.h
//
//		a random number generator
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <CoreFoundation/CoreFoundation.h>

#ifndef __TRandom
#define __TRandom

#define	kRandomSeed	161803398

UInt32	GetRandomLong(UInt32 inRange);
UInt32	GetRandomLong(UInt32 inLowerLimit, UInt32 inUpperLimit);


class TRandom
{
public:
	TRandom();
	TRandom(UInt32 n) {Seed(n);};
	
    void Seed(UInt32 n);

    UInt32 operator()(UInt32 inLimit) 
    {
		mIndex1 = (mIndex1 + 1) % 55;
		mIndex2 = (mIndex2 + 1) % 55;
		mTable[mIndex1] = mTable[mIndex1] - mTable[mIndex2];
		return mTable[mIndex1] % inLimit;
    };

protected:
    UInt32 mTable[55];
    long mIndex1;
    long mIndex2;
};

#endif		// __TRandom
