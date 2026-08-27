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
    void paint    (juce::Graphics&) override;
    void resized  () override;
    void mouseDown       (const juce::MouseEvent& event) override;
    void mouseUp         (const juce::MouseEvent& event) override;
    void mouseWheelMove  (const juce::MouseEvent& event,
                          const juce::MouseWheelDetails& wheel) override;
    bool keyPressed      (const juce::KeyPress& key) override;
    bool keyStateChanged (bool isKeyDown) override;

private:
    //==========================================================================
    // --- Helpers ---
    void updateChord      (std::vector<int> notes);
    void releaseBackground();

    //==========================================================================
    std::unique_ptr<juce::MidiOutput> midiOutput;

    // --- Strumming state (trackpad) ---
    const float       strumThreshold    { 0.1f };   // distancia mínima por paso
    float             scrollAccum       { 0.0f };   // acumulador de scroll
    int               strumIndex        { 0 };      // posición actual en el acorde
    int               lastNotePlayed    { -1 };     // nota de arpeggio activa
    std::vector<int>  currentChordNotes { 60, 64, 67, 72, 76, 79 }; // Do Mayor

    // --- Background state (teclado QWERTY) ---
    std::vector<int>  activeBackgroundNotes;         // notas de bajo sostenidas
    bool              keyAHeld { false };            // evita auto-repeat de A
    bool              keySHeld { false };            // evita auto-repeat de S

    // --- Botones de acorde ---
    juce::TextButton  btnCMajor { "C Major" };
    juce::TextButton  btnAMinor { "A Minor" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
