#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (600, 400);
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

    // --- Registrar botones ---
    addAndMakeVisible (btnCMajor);
    addAndMakeVisible (btnAMinor);

    // Lambda helper: conecta un botón a un acorde
    auto setupChordButton = [this](juce::TextButton& btn, std::vector<int> notes)
    {
        btn.onStateChange = [this, &btn, notes]()
        {
            if (btn.getState() == juce::Button::buttonDown)
            {
                if (midiOutput == nullptr) return;
                releaseBackground();
                updateChord (notes);
                for (int i = 0; i < 3; ++i)
                {
                    int note = currentChordNotes[(size_t) i] - 12;
                    midiOutput->sendMessageNow (
                        juce::MidiMessage::noteOn (1, note, (juce::uint8) 90));
                    activeBackgroundNotes.push_back (note);
                }
            }
            else if (btn.getState() == juce::Button::buttonNormal)
            {
                releaseBackground();
            }
        };
    };

    setupChordButton (btnCMajor, { 60, 64, 67, 72, 76, 79 });  // Do Mayor
    setupChordButton (btnAMinor, { 57, 60, 64, 69, 72, 76 });  // La Menor
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
    juce::FlexBox fb;
    fb.flexDirection  = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::center;
    fb.alignItems     = juce::FlexBox::AlignItems::center;

    fb.items.add (juce::FlexItem (btnCMajor).withWidth (100.f).withHeight (50.f)
                                            .withMargin ({ 0.f, 8.f, 0.f, 0.f }));
    fb.items.add (juce::FlexItem (btnAMinor).withWidth (100.f).withHeight (50.f));

    fb.performLayout (getLocalBounds().toFloat());
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
void MainComponent::mouseWheelMove (const juce::MouseEvent& /*event*/,
                                    const juce::MouseWheelDetails& wheel)
{
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
}

//==============================================================================
bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    if (midiOutput == nullptr) return false;

    // --- Do Mayor (A) ---
    if (key == juce::KeyPress ('a') && !keyAHeld)
    {
        keyAHeld = true;
        releaseBackground();
        updateChord ({ 60, 64, 67, 72, 76, 79 });   // C4 E4 G4 C5 E5 G5

        // Bajo: primeras 3 notas bajadas una octava
        for (int i = 0; i < 3; ++i)
        {
            int note = currentChordNotes[(size_t) i] - 12;
            midiOutput->sendMessageNow (juce::MidiMessage::noteOn (1, note, (juce::uint8) 90));
            activeBackgroundNotes.push_back (note);
        }
        return true;
    }

    // --- La Menor (S) ---
    if (key == juce::KeyPress ('s') && !keySHeld)
    {
        keySHeld = true;
        releaseBackground();
        updateChord ({ 57, 60, 64, 69, 72, 76 });   // A3 C4 E4 A4 C5 E5

        for (int i = 0; i < 3; ++i)
        {
            int note = currentChordNotes[(size_t) i] - 12;
            midiOutput->sendMessageNow (juce::MidiMessage::noteOn (1, note, (juce::uint8) 90));
            activeBackgroundNotes.push_back (note);
        }
        return true;
    }

    return false;
}

//==============================================================================
bool MainComponent::keyStateChanged (bool /*isKeyDown*/)
{
    // Detectar que A fue soltada
    if (keyAHeld && !juce::KeyPress::isKeyCurrentlyDown ('a'))
    {
        keyAHeld = false;
        releaseBackground();
    }
    // Detectar que S fue soltada
    if (keySHeld && !juce::KeyPress::isKeyCurrentlyDown ('s'))
    {
        keySHeld = false;
        releaseBackground();
    }
    return false;
}
