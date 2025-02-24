/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             LoopingAudioSampleBufferTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Explores audio sample buffer looping.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             Component
 mainClass:        MainContentComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once
#include "BouncingBall.h"
#include "ReferenceCountedBuffer.h"

//==============================================================================
class MainContentComponent   : public juce::AudioAppComponent, public juce::Timer
{
public:
    MainContentComponent()
    {
        addAndMakeVisible (openButton);
        openButton.setButtonText ("Open...");
        openButton.onClick = [this] { openButtonClicked(); };

        addAndMakeVisible (clearButton);
        clearButton.setButtonText ("Clear");
        clearButton.onClick = [this] { clearButtonClicked(); };

        setSize (300, 200);

        formatManager.registerBasicFormats();
        
        setAudioChannels (0, 2); // [7]
        
        startTimer(2000);
    }
    
    ~MainContentComponent() override
    {
        shutdownAudio();
    }

    void prepareToPlay (int, double) override {}

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        /** @TODO: Problem, in steps a), b) und c) um zu veranschaulichen, wann das Problem auftritt.*/
    
        const auto getBufferToUse = [&](){
            SpinLock::ScopedLockType sl{ currentBufferLock };
            return currentBuffer;
        };
        
        ReferenceCountedBuffer::Ptr bufferToUse = getBufferToUse();

        if (bufferToUse == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        
            
            
    
        
        // a) angenommen, wir bekommen tatsächlich in zwei Zeilen eine neue Referenz von currentBuffer
        
        /* b) ...   und jetzt, wo wir die Referenz haben, currentBuffer = nullptr von außen gesetzt wird und
                    bufferToUse nun die *einzige* Referenz ist
         */
        
        
        
        auto& fileBuffer = bufferToUse->getDataRef ();
        
        auto numInputChannels = fileBuffer.getNumChannels();
        auto numOutputChannels = bufferToFill.buffer->getNumChannels();
        
        auto outputSamplesRemaining = bufferToFill.numSamples;                                  // [8]
        auto outputSamplesOffset = bufferToFill.startSample;                                    // [9]
        
        while (outputSamplesRemaining > 0)
        {
            auto bufferSamplesRemaining = fileBuffer.getNumSamples() - position;                // [10]
            auto samplesThisTime = juce::jmin (outputSamplesRemaining, bufferSamplesRemaining); // [11]

            for (auto channel = 0; channel < numOutputChannels; ++channel)
            {
                bufferToFill.buffer->copyFrom (channel,                                         // [12]
                                               outputSamplesOffset,                             //  [12.1]
                                               fileBuffer,                                      //  [12.2]
                                               channel % numInputChannels,                      //  [12.3]
                                               position,                                        //  [12.4]
                                               samplesThisTime);                                //  [12.5]
            }

            outputSamplesRemaining -= samplesThisTime;                                          // [13]
            outputSamplesOffset += samplesThisTime;                                             // [14]
            position += samplesThisTime;                                                        // [15]

            if (position == fileBuffer.getNumSamples())
                position = 0;                                                                   // [16]
        }
        
        
        /** c)      ... und nun die letzte Referenz (bufferToUse) out of scope geht
                wird nun der gesamt ReferenceCountedBuffer **gelöscht** und zwar auf dem AudioThread.
                Ditte darf nicht passieren.
         
                **Wie könnte man das lösen?**
         */
    }

    void releaseResources() override
    {
        SpinLock::ScopedLockType slt{ currentBufferLock };
        currentBuffer = nullptr;
    }

    void resized() override
    {
        openButton .setBounds (10, 10, getWidth() - 20, 20);
        clearButton.setBounds (10, 40, getWidth() - 20, 20);
    }

private:
    BouncingBall ball { *this };
    
    void openButtonClicked()
    {
        chooser = std::make_unique<juce::FileChooser> ("Select a Wave file shorter than 2 seconds to play...",
                                                       juce::File{},
                                                       "*.wav");
        
        auto chooserFlags = juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();

            if (file == juce::File{})
                return;

            
            auto lambda = [/*capture list*/] (/*arguments*/) {/* function body */ };
            
            threads.addJob ([&, file]()
            {
                std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file)); // [2]

                jassert (reader.get() != nullptr);
                if (reader.get() != nullptr)
                {
                    auto duration = (float) reader->lengthInSamples / reader->sampleRate;           // [3]

                    ReferenceCountedBuffer::Ptr newBuffer = new ReferenceCountedBuffer (file.getFileNameWithoutExtension(), reader->numChannels, reader->lengthInSamples);

                    reader->read (&newBuffer->getDataRef(),                                                      // [5]
                                  0,                                                                //  [5.1]
                                  (int) reader->lengthInSamples,                                    //  [5.2]
                                  0,                                                                //  [5.3]
                                  true,                                                             //  [5.4]
                                  true);                                                            //  [5.5]
                    position = 0;                                                                   // [6]
                    
                    // add newBuffer to array to ensure that we have at least two references to our buffer
                    buffers.add (newBuffer);
                    
                    // lock the currentBufferLock to ensure that audio thread is not getting the reference at the same time
                    SpinLock::ScopedLockType slt{ currentBufferLock };
                    currentBuffer = newBuffer;
                }
            });
            
            
            
        });
    }

    void clearButtonClicked()
    {
        SpinLock::ScopedLockType slt{ currentBufferLock };
        currentBuffer = nullptr;
    }
    
    void timerCallback () override
    {
        // iterate over buffers and check for getReferenceCount () <= 1
        // remove buffers from array in case its unused
        for (int i = buffers.size (); --i >= 0;)
        {
            auto ptr = buffers.getUnchecked(i);
            
            if (ptr->getReferenceCount() <= 2)
                buffers.remove (i);
        }
    }

    ThreadPool threads;
    
    //==========================================================================
    juce::TextButton openButton;
    juce::TextButton clearButton;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;

    int position;

    ReferenceCountedBuffer::Ptr currentBuffer;
    SpinLock currentBufferLock;
    ReferenceCountedArray<ReferenceCountedBuffer> buffers;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
