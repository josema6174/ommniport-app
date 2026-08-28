#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setWantsKeyboardFocus (true);

    // Buscar el primer dispositivo MIDI que contenga "IAC" en su nombre
    for (auto& device : juce::MidiOutput::getAvailableDevices())
    {
        if (device.name.containsIgnoreCase ("IAC"))
        {
            midiOutput = juce::MidiOutput::openDevice (device.identifier);
            DBG ("Conectado a: " + device.name);
            break;
        }
    }

    // --- Tablas de datos musicales ---
    const juce::String rootNames[] = { "C","G","D","A","E","B","F#","C#","G#","D#","A#","F" };
    const int          rootMidi[]  = { 60, 67, 62, 69, 64, 71,  66,  61,  68,  63,  70, 65 };
    const juce::String typeNames[] = { "Major", "Minor", "Seventh" };
    const std::vector<int> typeIntervals[] = { {0,4,7}, {0,3,7}, {0,4,7,10} };

    // --- Doble bucle: tipo (fila) × raíz (columna) ---
    for (int t = 0; t < 3; ++t)
    {
        for (int r = 0; r < 12; ++r)
        {
            auto btn = std::make_unique<juce::TextButton> (rootNames[r] + " " + typeNames[t]);
            addAndMakeVisible (*btn);

            int root = rootMidi[r];
            std::vector<int> ivs = typeIntervals[t];

            // Guardar la definición del acorde para usarla desde el teclado
            chordDefs.push_back ({ root, ivs });
            int idx = (int) chordDefs.size() - 1;

            auto* rawBtn = btn.get();
            btn->onStateChange = [this, rawBtn, idx]()
            {
                if (rawBtn->getState() == juce::Button::buttonDown)
                    activateChord (idx);
                else if (rawBtn->getState() == juce::Button::buttonNormal)
                    releaseBackground();
            };

            chordButtons.push_back (std::move (btn));
        }
    }

    // --- Mapa teclado → índice de acorde ---
    // Distribución: fila Q/W/E... = Major, A/S/D... = Minor, Z/X/C... = Seventh
    // Columnas: C G D A E B F# C# G# D# (10 de las 12 raíces)
    const int majorKeys[]   = { 'q','w','e','r','t','y','u','i','o','p' };
    const int minorKeys[]   = { 'a','s','d','f','g','h','j','k','l',';' };
    const int sevKeys[]     = { 'z','x','c','v','b','n','m',',','.','/' };

    for (int r = 0; r < 10; ++r)
    {
        keyToChordIndex[majorKeys[r]] = 0 * 12 + r;   // fila Major
        keyToChordIndex[minorKeys[r]] = 1 * 12 + r;   // fila Minor
        keyToChordIndex[sevKeys[r]]   = 2 * 12 + r;   // fila Seventh
    }

    // setSize al final: así resized() se llama con chordButtons ya poblado
    setSize (900, 300);
}

MainComponent::~MainComponent() {}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // Fondo oscuro
    g.fillAll (juce::Colour (0xff1e1e2e));

    // Placa de rasgueo
    g.setColour (juce::Colours::darkgrey);
    g.fillRect (strumZone);
}

//==============================================================================
void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    strumZone   = bounds.removeFromRight (bounds.getWidth() / 3);

    juce::Grid grid;
    grid.rowGap    = juce::Grid::Px (3);
    grid.columnGap = juce::Grid::Px (3);

    for (int i = 0; i < 3;  ++i)
        grid.templateRows.add    (juce::Grid::TrackInfo (juce::Grid::Fr (1)));
    for (int i = 0; i < 12; ++i)
        grid.templateColumns.add (juce::Grid::TrackInfo (juce::Grid::Fr (1)));

    for (auto& btn : chordButtons)
        grid.items.add (juce::GridItem (*btn));

    grid.performLayout (bounds);
}

//==============================================================================
void MainComponent::mouseDown (const juce::MouseEvent& /*event*/)
{
    if (midiOutput != nullptr)
        midiOutput->sendMessageNow (
            juce::MidiMessage::noteOn (1, 60, (juce::uint8) 127));
}

void MainComponent::mouseUp (const juce::MouseEvent& /*event*/)
{
    if (midiOutput != nullptr)
        midiOutput->sendMessageNow (
            juce::MidiMessage::noteOff (1, 60));
}

//==============================================================================
void MainComponent::mouseWheelMove (const juce::MouseEvent& event,
                                    const juce::MouseWheelDetails& wheel)
{
    if (! strumZone.contains (event.getPosition()))
        return;

    if (midiOutput == nullptr)
        return;

    // Acumular desplazamiento (preferir deltaY; usar deltaX si no hay deltaY)
    float delta = (wheel.deltaY != 0.0f) ? wheel.deltaY : wheel.deltaX;
    scrollAccum += delta;

    // Disparar un paso de rasgueo por cada vez que superamos el umbral
    while (std::abs (scrollAccum) >= strumThreshold)
    {
        // 1. Apagar nota anterior
        if (lastNotePlayed != -1)
            midiOutput->sendMessageNow (juce::MidiMessage::noteOff (1, lastNotePlayed));

        // 2. Avanzar índice según dirección del scroll
        if (scrollAccum > 0.0f)
            strumIndex = (strumIndex + 1) % (int) currentChordNotes.size();
        else
            strumIndex = (strumIndex - 1 + (int) currentChordNotes.size()) % (int) currentChordNotes.size();

        // 3. Encender nueva nota del acorde
        int note = currentChordNotes[(size_t) strumIndex];
        midiOutput->sendMessageNow (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100));
        lastNotePlayed = note;

        // 4. Consumir un paso del acumulador
        scrollAccum -= (scrollAccum > 0.0f ? strumThreshold : -strumThreshold);
    }
}

//==============================================================================
void MainComponent::updateChord (std::vector<int> notes)
{
    // Apagar nota de arpeggio si estaba sonando
    if (lastNotePlayed != -1 && midiOutput != nullptr)
    {
        midiOutput->sendMessageNow (juce::MidiMessage::noteOff (1, lastNotePlayed));
        lastNotePlayed = -1;
    }
    currentChordNotes = std::move (notes);
    strumIndex        = 0;
    scrollAccum       = 0.0f;
}

void MainComponent::releaseBackground()
{
    if (midiOutput == nullptr) return;
    for (int n : activeBackgroundNotes)
        midiOutput->sendMessageNow (juce::MidiMessage::noteOff (1, n));
    activeBackgroundNotes.clear();

    // Restaurar color del botón activo
    if (activeButtonIndex >= 0 && activeButtonIndex < (int) chordButtons.size())
        chordButtons[(size_t) activeButtonIndex]->removeColour (juce::TextButton::buttonColourId);
    activeButtonIndex = -1;
}

//==============================================================================
bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    if (midiOutput == nullptr) return false;

    int kc = key.getKeyCode();

    // Diagnóstico: muestra el código real que llega (visible en Debug Console)
    DBG ("keyPressed  raw=" + juce::String (kc)
         + "  char='" + juce::String::charToString ((juce_wchar) kc) + "'");

    // Normalizar letras a minúscula: macOS puede enviar 'A'=65 en vez de 'a'=97
    if (juce::CharacterFunctions::isLetter ((juce_wchar) kc))
        kc = (int) juce::CharacterFunctions::toLowerCase ((juce_wchar) kc);

    auto it = keyToChordIndex.find (kc);
    if (it == keyToChordIndex.end()) return false;

    if (kc == activeKeyCode) return true;   // ignorar auto-repeat del SO
    activeKeyCode = kc;
    activateChord (it->second);
    return true;
}

//==============================================================================
bool MainComponent::keyStateChanged (bool /*isKeyDown*/)
{
    if (activeKeyCode != -1
        && ! juce::KeyPress::isKeyCurrentlyDown (activeKeyCode))
    {
        activeKeyCode = -1;
        releaseBackground();
    }
    return false;
}

//==============================================================================
void MainComponent::activateChord (int idx)
{
    if (midiOutput == nullptr) return;
    const auto& def = chordDefs[(size_t) idx];

    // Construir arpeggio: raíz + intervalos × 4 octavas
    std::vector<int> notes;
    for (int oct = 0; oct < 4; ++oct)
        for (int iv : def.intervals)
            if (int n = def.root + oct * 12 + iv; n <= 127)
                notes.push_back (n);

    releaseBackground();   // también resetea el color del botón anterior
    updateChord (notes);

    // Resaltar el botón del acorde activo en celeste
    activeButtonIndex = idx;
    chordButtons[(size_t) idx]->setColour (juce::TextButton::buttonColourId,
                                           juce::Colour (0xffb3e5fc));

    // Background: primeras 3 notas − 1 octava
    for (int i = 0; i < 3 && i < (int) currentChordNotes.size(); ++i)
    {
        int note = currentChordNotes[(size_t) i] - 12;
        midiOutput->sendMessageNow (
            juce::MidiMessage::noteOn (1, note, (juce::uint8) 90));
        activeBackgroundNotes.push_back (note);
    }
}
