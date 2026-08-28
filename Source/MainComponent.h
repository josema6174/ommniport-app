#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * MainComponent implementa la interfaz auténtica del Omnichord (estilo OM-27 / OM-36)
 * con matriz inclinada (sheared parallelogram) y placa de rasgueo táctil.
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
    void mouseDrag       (const juce::MouseEvent& event) override;
    void mouseUp         (const juce::MouseEvent& event) override;
    void mouseWheelMove  (const juce::MouseEvent& event,
                          const juce::MouseWheelDetails& wheel) override;
    bool keyPressed      (const juce::KeyPress& key) override;
    bool keyStateChanged (bool isKeyDown) override;

private:
    //==========================================================================
    // --- Helpers de acordes y MIDI ---
    void updateChord       (std::vector<int> notes);
    void releaseBackground ();
    void activateChord     (int buttonIndex);
    int  getChordIndexAt   (juce::Point<float> pt) const;

    //==========================================================================
    std::unique_ptr<juce::MidiOutput> midiOutput;

    // --- Geometría de interfaz ---
    juce::Rectangle<float> matrixArea;
    juce::Rectangle<int>   strumZone;
    const float            slantDx { 34.0f };       // Inclinación horizontal de columnas

    // --- Strumming state (trackpad) ---
    const float       strumThreshold    { 0.08f };  // distancia mínima por paso
    float             scrollAccum       { 0.0f };   // acumulador de scroll
    int               strumIndex        { 0 };      // posición actual en el acorde
    int               lastNotePlayed    { -1 };     // nota de arpeggio activa
    std::vector<int>  currentChordNotes { 60, 64, 67, 72, 76, 79, 84, 88, 91, 96, 100, 103 };

    // --- Background state ---
    std::vector<int>  activeBackgroundNotes;        // notas de bajo sostenidas

    // --- Chord definitions (9 columnas x 3 filas = 27 acordes) ---
    struct ChordDef { int root; std::vector<int> intervals; juce::String name; };
    std::vector<ChordDef>   chordDefs;
    std::map<int, int>      keyToChordIndex;        // keyCode -> chord index (0..26)
    int                     activeKeyCode     { -1 };
    int                     activeButtonIndex { -1 };
    bool                    isMouseDownOnMatrix { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

