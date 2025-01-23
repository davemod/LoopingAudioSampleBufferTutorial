/*
  ==============================================================================

    BouncingBall.h
    Created: 23 Jan 2025 8:16:24pm
    Author:  David Hill

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class BouncingBall : public Component, public Timer
{
public:
    BouncingBall (Component& parentComponent)
    {
        addToDesktop (ComponentPeer::StyleFlags::windowIgnoresMouseClicks);
        setVisible (true);
        setAlwaysOnTop(true);
        setBounds (5,5,25,25);
        startTimerHz(60);
    }
    
private:
    Point<int> direction{1, 5};
    
    void paint (Graphics& g) override
    {
        const auto bounds = getLocalBounds ();
        const auto size = jmin (bounds.getWidth (), bounds.getHeight ());
        
        g.setColour (Colours::red.withAlpha (0.7f));
        g.fillEllipse (bounds.toFloat ().withSizeKeepingCentre (size, size));
    }
    
    void timerCallback () override
    {
        const auto getLimit = [&]() -> Rectangle<int> {
            if (auto parent = getParentComponent ())
                return parent->getLocalBounds ();
            
            if (auto display = Desktop::getInstance ().getDisplays ().getPrimaryDisplay ())
                return display->userArea;
            
            return {};
        };
        
        const auto parentBounds = getLimit ();
        const auto bounds = getBoundsInParent ();
        
        if (parentBounds.isEmpty ())
            return;
        
        if (bounds.getRight () >= parentBounds.getRight () || bounds.getX () <= parentBounds.getX ())
            direction.x *= -1;
        
        if (bounds.getY () <= parentBounds.getY () || bounds.getBottom () >= parentBounds.getBottom ())
            direction.y *= -1;
        
        setBounds (bounds.translated (direction.x, direction.y));
        
    }
};
