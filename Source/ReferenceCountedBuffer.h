/*
  ==============================================================================

    ReferenceCountedBuffer.h
    Created: 24 Jan 2025 11:10:34am
    Author:  HFM

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ReferenceCountedBuffer : public ReferenceCountedObject
{
public:
    ReferenceCountedBuffer (const String& name, int numChannels, int numSamples)
    :
    name (name),
    data (numChannels, numSamples)
    {
        DBG ("Created buffer: " << name);
    }
    
    ~ReferenceCountedBuffer ()
    {
        DBG ("Deleted buffer: " << name);
    }
    
    AudioBuffer<float>& getDataRef () { return data; }
    
    using Ptr = ReferenceCountedObjectPtr<ReferenceCountedBuffer>;
    
private:
    const String name;
    AudioBuffer<float> data;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReferenceCountedBuffer)
};
