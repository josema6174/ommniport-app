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

    // --- Tablas de datos musicales estilo Omnichord (9 columnas x 3 filas = 27 acordes) ---
    const juce::String rootNames[] = { "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B" };
    const int          rootMidi[]  = { 63,   70,   65,  60,  67,  62,  69,  64,  71 };
    const juce::String typeNames[] = { "Major", "Minor", "7th" };
    const std::vector<int> typeIntervals[] = { {0, 4, 7}, {0, 3, 7}, {0, 4, 7, 10} };

    for (int t = 0; t < 3; ++t)
    {
        for (int r = 0; r < 9; ++r)
        {
            juce::String chordName = rootNames[r] + " " + typeNames[t];
            chordDefs.push_back ({ rootMidi[r], typeIntervals[t], chordName });
        }
    }

    // --- Mapa teclado QWERTY (9 teclas por fila) ---
    // Major:   Q W E R T Y U I O
    // Minor:   A S D F G H J K L
    // 7th:     Z X C V B N M , .
    const int majorKeys[] = { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o' };
    const int minorKeys[] = { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l' };
    const int sevKeys[]   = { 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.' };

    for (int r = 0; r < 9; ++r)
    {
        keyToChordIndex[majorKeys[r]] = 0 * 9 + r;  // Fila MAJOR
        keyToChordIndex[minorKeys[r]] = 1 * 9 + r;  // Fila MINOR
        keyToChordIndex[sevKeys[r]]   = 2 * 9 + r;  // Fila 7TH
    }

    // Tamaño inicial cómodo para la matriz y la placa de rasgueo
    setSize (980, 340);
}

MainComponent::~MainComponent()
{
    releaseBackground();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // Fondo general estilo chasis Omnichord (crema / marfil retro)
    g.fillAll (juce::Colour (0xffeef0f3));

    const auto navyBlue  = juce::Colour (0xff244576);
    const auto activeSky = juce::Colour (0xffbde5fc);

    float x0 = matrixArea.getX();
    float y0 = matrixArea.getY();
    float w  = matrixArea.getWidth();
    float h  = matrixArea.getHeight();
    float s  = slantDx;

    // --- 1. Contenedor exterior de la Matriz (Paralelogramo con esquinas redondeadas) ---
    juce::Path matrixPath;
    float rCorner = 18.0f;

    // Construcción del contorno suave inclinado
    matrixPath.startNewSubPath (x0 + rCorner, y0);
    matrixPath.lineTo (x0 + w - s - rCorner, y0);
    matrixPath.quadraticTo (x0 + w - s, y0, x0 + w - s + (rCorner * s / h), y0 + rCorner);
    matrixPath.lineTo (x0 + w - (rCorner * s / h), y0 + h - rCorner);
    matrixPath.quadraticTo (x0 + w, y0 + h, x0 + w - rCorner, y0 + h);
    matrixPath.lineTo (x0 + s + rCorner, y0 + h);
    matrixPath.quadraticTo (x0 + s, y0 + h, x0 + s - (rCorner * s / h), y0 + h - rCorner);
    matrixPath.lineTo (x0 + (rCorner * s / h), y0 + rCorner);
    matrixPath.quadraticTo (x0, y0, x0 + rCorner, y0);
    matrixPath.closeSubPath();

    // Relleno blanco marfil y sombra exterior sutil
    g.setColour (juce::Colour (0x12000000));
    g.fillPath (matrixPath, juce::AffineTransform::translation (2.0f, 3.0f));

    g.setColour (juce::Colours::white);
    g.fillPath (matrixPath);

    // --- 2. Celdas activas (resaltado celeste suave) ---
    float headerH   = h * 0.18f;
    float contentH  = h - headerH;
    float rowH      = contentH / 3.0f;
    float labelColW = 88.0f;
    float colW      = (w - s - labelColW) / 9.0f;

    if (activeButtonIndex >= 0 && activeButtonIndex < (int) chordDefs.size())
    {
        float actRow = (float) (activeButtonIndex / 9);
        float actCol = (float) (activeButtonIndex % 9);

        float yTop = y0 + headerH + actRow * rowH;
        float yBot = yTop + rowH;
        float yNormTop = (yTop - y0) / h;
        float yNormBot = (yBot - y0) / h;

        float xL_top = x0 + labelColW + actCol * colW + yNormTop * s;
        float xR_top = xL_top + colW;
        float xL_bot = x0 + labelColW + actCol * colW + yNormBot * s;
        float xR_bot = xL_bot + colW;

        juce::Path cellHighlight;
        cellHighlight.startNewSubPath (xL_top + 2.0f, yTop + 2.0f);
        cellHighlight.lineTo (xR_top - 2.0f, yTop + 2.0f);
        cellHighlight.lineTo (xR_bot - 2.0f, yBot - 2.0f);
        cellHighlight.lineTo (xL_bot + 2.0f, yBot - 2.0f);
        cellHighlight.closeSubPath();

        g.setColour (activeSky);
        g.fillPath (cellHighlight);
    }

    // --- 3. Líneas divisorias inclinadas entre columnas ---
    g.setColour (navyBlue);

    // Línea separadora entre etiquetas de fila y primera columna
    float xSepTop = x0 + labelColW;
    float xSepBot = xSepTop + s;
    g.drawLine (xSepTop, y0, xSepBot, y0 + h, 2.0f);

    // Líneas entre las 9 columnas de notas
    for (int c = 1; c < 9; ++c)
    {
        float xTop = x0 + labelColW + (float) c * colW;
        float xBot = xTop + s;
        g.drawLine (xTop, y0, xBot, y0 + h, 2.0f);
    }

    // Trazo de borde exterior del panel
    g.setColour (navyBlue);
    g.strokePath (matrixPath, juce::PathStrokeType (2.5f));

    // --- 4. Etiquetas de cabecera de notas raíz (Eb, Bb, F, C, G, D, A, E, B) ---
    const juce::String rootNames[] = { "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B" };
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));

    for (int c = 0; c < 9; ++c)
    {
        float xCenterTop = x0 + labelColW + ((float) c + 0.5f) * colW + (headerH * 0.5f / h) * s;
        juce::Rectangle<float> headerRect (xCenterTop - 25.0f, y0 + 6.0f, 50.0f, headerH - 8.0f);
        g.drawText (rootNames[c], headerRect, juce::Justification::centred, false);
    }

    // --- 5. Etiquetas de tipo de acorde a la izquierda (MAJOR, MINOR, 7TH) ---
    const juce::String rowLabels[] = { "MAJOR", "MINOR", "7TH" };
    g.setFont (juce::FontOptions (15.5f, juce::Font::bold));

    for (int r = 0; r < 3; ++r)
    {
        float yCenter = y0 + headerH + ((float) r + 0.5f) * rowH;
        float yNorm   = (yCenter - y0) / h;
        float xLeft   = x0 + yNorm * s + 10.0f;

        juce::Rectangle<float> labelRect (xLeft, yCenter - 14.0f, labelColW - 16.0f, 28.0f);
        g.drawText (rowLabels[r], labelRect, juce::Justification::centredLeft, false);
    }

    // --- 6. Marcas táctiles clásicas '_|' en cada casilla de acorde ---
    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 9; ++c)
        {
            float yTop = y0 + headerH + (float) r * rowH;
            float yBot = yTop + rowH;
            float yNormCenter = ((yTop + yBot) * 0.5f - y0) / h;

            float cx = x0 + labelColW + ((float) c + 0.5f) * colW + yNormCenter * s + 6.0f;
            float cy = (yTop + yBot) * 0.5f + 4.0f;

            // Relieve 3D sutil (sombra blanca)
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.drawLine (cx - 15.0f, cy + 9.0f, cx + 9.0f, cy + 9.0f, 2.5f);
            g.drawLine (cx + 9.0f, cy + 9.0f, cx + 9.0f, cy - 13.0f, 2.5f);

            // Trazo azul principal
            g.setColour (navyBlue);
            g.drawLine (cx - 16.0f, cy + 8.0f, cx + 8.0f, cy + 8.0f, 2.5f);
            g.drawLine (cx + 8.0f, cy + 8.0f, cx + 8.0f, cy - 14.0f, 2.5f);
        }
    }

    // --- 7. Placa de Rasgueo ("STRUMPLATE") estilo retro auténtico ---
    if (! strumZone.isEmpty())
    {
        auto sz = strumZone.toFloat();

        // Placa base metálica oscura con esquinas redondeadas
        juce::Path strumPlate;
        strumPlate.addRoundedRectangle (sz, 14.0f);

        juce::ColourGradient plateGrad (juce::Colour (0xff2d3748), sz.getX(), sz.getY(),
                                        juce::Colour (0xff1a202c), sz.getX(), sz.getBottom(), false);
        g.setGradientFill (plateGrad);
        g.fillPath (strumPlate);

        // Borde exterior
        g.setColour (navyBlue);
        g.strokePath (strumPlate, juce::PathStrokeType (2.5f));

        // Rótulo STRUMPLATE
        g.setColour (juce::Colour (0xffcbd5e1));
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("OMNICHORD STRUMPLATE", sz.removeFromTop (26.0f), juce::Justification::centred, false);

        // Tiras táctiles metálicas doradas/plateadas
        auto barArea = sz.reduced (16.0f, 10.0f);
        int numBars = 12;
        float barSpacing = barArea.getWidth() / (float) numBars;

        for (int i = 0; i < numBars; ++i)
        {
            float bx = barArea.getX() + ((float) i + 0.5f) * barSpacing;
            g.setColour (juce::Colour (0xffd4af37).withAlpha (0.75f)); // Dorado táctil
            g.drawLine (bx, barArea.getY(), bx, barArea.getBottom(), 3.0f);

            // Brillo central de cada pista
            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.drawLine (bx, barArea.getY() + 4.0f, bx, barArea.getBottom() - 4.0f, 1.0f);
        }
    }
}

//==============================================================================
void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Área derecha: Placa de rasgueo (220 px)
    strumZone = bounds.removeFromRight (220).reduced (10, 16);

    // Área izquierda: Matriz de acordes
    matrixArea = bounds.toFloat().reduced (14.0f, 16.0f);
}

//==============================================================================
int MainComponent::getChordIndexAt (juce::Point<float> pt) const
{
    float x0 = matrixArea.getX();
    float y0 = matrixArea.getY();
    float w  = matrixArea.getWidth();
    float h  = matrixArea.getHeight();
    float s  = slantDx;

    if (pt.y < y0 || pt.y > y0 + h)
        return -1;

    float headerH = h * 0.18f;
    if (pt.y < y0 + headerH)
        return -1;

    float rowH = (h - headerH) / 3.0f;
    int row = (int) ((pt.y - (y0 + headerH)) / rowH);
    if (row < 0 || row >= 3)
        return -1;

    float yNorm = (pt.y - y0) / h;
    float xShift = yNorm * s;
    float labelColW = 88.0f;
    float colW = (w - s - labelColW) / 9.0f;

    float relX = pt.x - (x0 + xShift + labelColW);
    if (relX < 0.0f)
        return -1;

    int col = (int) (relX / colW);
    if (col < 0 || col >= 9)
        return -1;

    return row * 9 + col;
}

//==============================================================================
void MainComponent::mouseDown (const juce::MouseEvent& event)
{
    auto pt = event.position;
    int chordIdx = getChordIndexAt (pt);
    if (chordIdx >= 0)
    {
        isMouseDownOnMatrix = true;
        activateChord (chordIdx);
        repaint();
        return;
    }

    if (strumZone.contains (event.getPosition()))
    {
        if (midiOutput != nullptr && ! currentChordNotes.empty())
        {
            int note = currentChordNotes[(size_t) strumIndex];
            midiOutput->sendMessageNow (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100));
            lastNotePlayed = note;
        }
    }
}

void MainComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (isMouseDownOnMatrix)
    {
        int chordIdx = getChordIndexAt (event.position);
        if (chordIdx >= 0 && chordIdx != activeButtonIndex)
        {
            activateChord (chordIdx);
            repaint();
        }
    }
}

void MainComponent::mouseUp (const juce::MouseEvent& /*event*/)
{
    if (isMouseDownOnMatrix)
    {
        isMouseDownOnMatrix = false;
        releaseBackground();
        repaint();
    }

    if (lastNotePlayed != -1 && midiOutput != nullptr)
    {
        midiOutput->sendMessageNow (juce::MidiMessage::noteOff (1, lastNotePlayed));
        lastNotePlayed = -1;
    }
}

//==============================================================================
void MainComponent::mouseWheelMove (const juce::MouseEvent& event,
                                    const juce::MouseWheelDetails& wheel)
{
    if (! strumZone.contains (event.getPosition()))
        return;

    if (midiOutput == nullptr || currentChordNotes.empty())
        return;

    float delta = (wheel.deltaY != 0.0f) ? wheel.deltaY : wheel.deltaX;
    scrollAccum += delta;

    while (std::abs (scrollAccum) >= strumThreshold)
    {
        if (lastNotePlayed != -1)
            midiOutput->sendMessageNow (juce::MidiMessage::noteOff (1, lastNotePlayed));

        if (scrollAccum > 0.0f)
            strumIndex = (strumIndex + 1) % (int) currentChordNotes.size();
        else
            strumIndex = (strumIndex - 1 + (int) currentChordNotes.size()) % (int) currentChordNotes.size();

        int note = currentChordNotes[(size_t) strumIndex];
        midiOutput->sendMessageNow (juce::MidiMessage::noteOn (1, note, (juce::uint8) 105));
        lastNotePlayed = note;

        scrollAccum -= (scrollAccum > 0.0f ? strumThreshold : -strumThreshold);
    }
}

//==============================================================================
void MainComponent::updateChord (std::vector<int> notes)
{
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
    if (midiOutput != nullptr)
    {
        for (int n : activeBackgroundNotes)
            midiOutput->sendMessageNow (juce::MidiMessage::noteOff (1, n));
    }
    activeBackgroundNotes.clear();
    activeButtonIndex = -1;
    repaint();
}

//==============================================================================
bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    if (midiOutput == nullptr) return false;

    int kc = key.getKeyCode();

    // Normalizar letras a minúscula (macOS compatibility)
    if (juce::CharacterFunctions::isLetter ((juce_wchar) kc))
        kc = (int) juce::CharacterFunctions::toLowerCase ((juce_wchar) kc);

    auto it = keyToChordIndex.find (kc);
    if (it == keyToChordIndex.end()) return false;

    if (kc == activeKeyCode) return true; // Ignorar auto-repeat
    activeKeyCode = kc;
    activateChord (it->second);
    return true;
}

bool MainComponent::keyStateChanged (bool /*isKeyDown*/)
{
    if (activeKeyCode != -1 && ! juce::KeyPress::isKeyCurrentlyDown (activeKeyCode))
    {
        activeKeyCode = -1;
        releaseBackground();
    }
    return false;
}

//==============================================================================
void MainComponent::activateChord (int idx)
{
    if (idx < 0 || idx >= (int) chordDefs.size())
        return;

    const auto& def = chordDefs[(size_t) idx];

    // Construir arpeggio: 4 octavas
    std::vector<int> notes;
    for (int oct = 0; oct < 4; ++oct)
    {
        for (int iv : def.intervals)
        {
            int n = def.root + oct * 12 + iv;
            if (n <= 127)
                notes.push_back (n);
        }
    }

    releaseBackground();
    updateChord (notes);

    activeButtonIndex = idx;
    repaint();

    // Sonar bajo de fondo: primeras 3 notas bajadas 1 octava (-12)
    if (midiOutput != nullptr)
    {
        for (int i = 0; i < 3 && i < (int) currentChordNotes.size(); ++i)
        {
            int note = currentChordNotes[(size_t) i] - 12;
            midiOutput->sendMessageNow (juce::MidiMessage::noteOn (1, note, (juce::uint8) 92));
            activeBackgroundNotes.push_back (note);
        }
    }
}

