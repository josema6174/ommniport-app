#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * MainComponent es el componente raíz de la ventana principal.
 * Tamaño inicial: 600 x 400 px.
 */
class MainComponent : public juce::Component
{
public:
    //==========================================================================
    MainComponent();
    ~MainComponent() override;

    //==========================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
