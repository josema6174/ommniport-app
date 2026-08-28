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
    void activateChord    (int buttonIndex);

    //==========================================================================
    std::unique_ptr<juce::MidiOutput> midiOutput;

    // --- Strumming state (trackpad) ---
    juce::Rectangle<int>  strumZone;                 // área táctil de rasgueo
    const float       strumThreshold    { 0.1f };   // distancia mínima por paso
    float             scrollAccum       { 0.0f };   // acumulador de scroll
    int               strumIndex        { 0 };      // posición actual en el acorde
    int               lastNotePlayed    { -1 };     // nota de arpeggio activa
    std::vector<int>  currentChordNotes { 60, 64, 67, 72, 76, 79 }; // Do Mayor

    // --- Background state ---
    std::vector<int>  activeBackgroundNotes;         // notas de bajo sostenidas

    // --- Keyboard mapping ---
    struct ChordDef { int root; std::vector<int> intervals; };
    std::vector<ChordDef>   chordDefs;              // paralelo a chordButtons
    std::map<int, int>      keyToChordIndex;        // keyCode → índice de botón
    int                     activeKeyCode    { -1 };// tecla activa (-1 = ninguna)
    int                     activeButtonIndex{ -1 };// botón destacado (-1 = ninguno)

    // --- Matriz de botones de acorde (36 = 12 raíces × 3 tipos) ---
    std::vector<std::unique_ptr<juce::TextButton>> chordButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
