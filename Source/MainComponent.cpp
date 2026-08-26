#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (600, 400);
}

MainComponent::~MainComponent() {}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // Fondo oscuro
    g.fillAll (juce::Colour (0xff1e1e2e));

    // Texto de bienvenida centrado
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f));
    g.drawText ("OmniportMIDI",
                getLocalBounds(),
                juce::Justification::centred,
                true);
}

//==============================================================================
void MainComponent::resized()
{
    // Aquí se posicionarán los subcomponentes cuando se agreguen.
}
