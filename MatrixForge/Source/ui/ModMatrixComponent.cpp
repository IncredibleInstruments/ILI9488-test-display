#include "ModMatrixComponent.h"
#include "ForgeLookAndFeel.h"

namespace mf
{
ModMatrixComponent::ModMatrixComponent (juce::AudioProcessorValueTreeState& apvts, std::function<float (int)> sourceValue)
    : state (apvts), getSourceValue (std::move (sourceValue))
{
    for (int s = 0; s < kNumModSources; ++s)
    {
        for (int d = 0; d < kNumModDests; ++d)
        {
            auto& cell = cells[s][d];
            cell.param = apvts.getParameter (matrixId (s, d));
            cell.attachment = std::make_unique<juce::ParameterAttachment> (*cell.param, [this, &cell] (float v)
            {
                cell.value = v;
                repaint();
            });
            cell.attachment->sendInitialUpdate();
        }
    }
    startTimerHz (30);
}

float ModMatrixComponent::param (const char* id) const
{
    return state.getRawParameterValue (id)->load();
}

juce::String ModMatrixComponent::explainSilentRoute (int source, int dest) const
{
    auto vcoSilent = [this] (int n) -> juce::String
    {
        const juce::String p = "vco" + juce::String (n);
        if (param (("mixVco" + juce::String (n)).toRawUTF8()) < 0.01f)
            return "VCO " + juce::String (n) + " is turned down in the mixer";
        if (param ((p + "Saw").toRawUTF8()) + param ((p + "Square").toRawUTF8()) + param ((p + "Tri").toRawUTF8()) + param ((p + "Sub").toRawUTF8()) < 0.01f)
            return "VCO " + juce::String (n) + " has every waveform at 0";
        return {};
    };

    auto feeds = [this] (bool steiner)
    {
        const char* mixes[] { "mixVco1", "mixVco2", "mixVco3", "mixNoise" };
        const char* routes[] { "routeVco1", "routeVco2", "routeVco3", "routeNoise" };
        for (int i = 0; i < 4; ++i)
        {
            const int r = (int) param (routes[i]);
            if (param (mixes[i]) > 0.01f && (steiner ? r <= 1 : r >= 1))
                return true;
        }
        return false;
    };
    const bool series = param ("filterConfig") >= 0.5f;
    const bool steinerHeard = feeds (true) && (series ? param ("stOut") > 0.01f && param ("ldOut") > 0.01f : param ("stOut") > 0.01f);
    const bool ladderHeard = (feeds (false) || (series && feeds (true) && param ("stOut") > 0.01f)) && param ("ldOut") > 0.01f;

    juce::String why;
    switch (dest)
    {
        case dVco1Pitch: why = vcoSilent (1); break;
        case dVco2Pitch: why = vcoSilent (2); break;
        case dVco1PW:
            why = vcoSilent (1);
            if (why.isEmpty() && param ("vco1Square") < 0.01f) why = "VCO 1 Square level is 0 - pulse width only changes the square wave";
            break;
        case dVco2PW:
            why = vcoSilent (2);
            if (why.isEmpty() && param ("vco2Square") < 0.01f) why = "VCO 2 Square level is 0 - pulse width only changes the square wave";
            break;
        case dVco1Metal:
            why = vcoSilent (1);
            if (why.isEmpty() && param ("vco1Tri") < 0.01f) why = "VCO 1 Triangle level is 0 - the Metalizer folds the triangle wave";
            break;
        case dVco2Metal:
            why = vcoSilent (2);
            if (why.isEmpty() && param ("vco2Tri") < 0.01f) why = "VCO 2 Triangle level is 0 - the Metalizer folds the triangle wave";
            break;
        case dUltra:
            if (param ("vco1Saw") * param ("mixVco1") + param ("vco2Saw") * param ("mixVco2") < 0.01f)
                why = "No saw wave is playing - Ultrasaw only affects the saw waves";
            break;
        case dVco3Pitch:
        {
            bool used = param ("mixVco3") > 0.01f || param ("vco1FM") > 0.01f;
            for (int d = 0; d < kNumModDests && ! used; ++d)
                used = std::abs (cells[srcVco3][d].value) > 0.001f;
            if (! used) why = "VCO 3 is silent and not modulating anything";
            break;
        }
        case dStCutoff: case dStReso: case dBrute:
            if (! steinerHeard) why = "Nothing audible passes through the Steiner filter (check the mixer routing / Steiner output)";
            break;
        case dLdCutoff: case dLdReso:
            if (! ladderHeard) why = "Nothing audible passes through the Ladder filter (check the mixer routing / Ladder output)";
            break;
        default: break;
    }

    if (why.isEmpty())
    {
        if (source == srcWheel) why = "Move the mod wheel to hear this route";
        else if (source == srcAftertouch) why = "Press harder on the keys (aftertouch) to hear this route";
        else if (source == srcVelocity) why = "Depends on how hard you play: soft notes push the opposite way";
        else if (source == srcEnv3 && param ("env3S") > 0.99f && param ("env3A") < 0.01f) why = "Env 3 jumps straight to full sustain - lengthen its attack or lower sustain for movement";
    }
    return why;
}

void ModMatrixComponent::timerCallback()
{
    bool changed = false;
    for (int s = 0; s < kNumModSources; ++s)
    {
        const float v = getSourceValue ? getSourceValue (s) : 0.0f;
        if (std::abs (v - liveSources[s]) > 0.004f)
        {
            liveSources[s] = v;
            changed = true;
        }
    }
    if (changed)
        repaint();
}

juce::Rectangle<float> ModMatrixComponent::cellBounds (int source, int dest) const
{
    const float cw = ((float) getWidth() - labelWidth) / (float) kNumModDests;
    const float ch = ((float) getHeight() - headerHeight - footerHeight) / (float) kNumModSources;
    return { labelWidth + (float) dest * cw, headerHeight + (float) source * ch, cw, ch };
}

bool ModMatrixComponent::cellAt (juce::Point<float> p, int& source, int& dest) const
{
    const float gridBottom = (float) getHeight() - footerHeight;
    if (p.x < labelWidth || p.y < headerHeight || p.y >= gridBottom)
        return false;
    const float cw = ((float) getWidth() - labelWidth) / (float) kNumModDests;
    const float ch = (gridBottom - headerHeight) / (float) kNumModSources;
    dest = juce::jlimit (0, kNumModDests - 1, (int) ((p.x - labelWidth) / cw));
    source = juce::jlimit (0, kNumModSources - 1, (int) ((p.y - headerHeight) / ch));
    return true;
}

void ModMatrixComponent::paint (juce::Graphics& g)
{
    const juce::Font groupFont (juce::FontOptions (9.0f, juce::Font::bold));
    const juce::Font labelFont (juce::FontOptions (8.5f, juce::Font::bold));

    // Column groups
    for (int d = 0; d < kNumModDests;)
    {
        const int group = modDestGroup (d);
        int end = d;
        while (end + 1 < kNumModDests && modDestGroup (end + 1) == group)
            ++end;
        auto first = cellBounds (0, d), last = cellBounds (0, end);
        auto band = juce::Rectangle<float> (first.getX() + 1.0f, 0.0f, last.getRight() - first.getX() - 2.0f, groupHeight - 1.0f);
        g.setColour (colours::panelEdge.withAlpha (group % 2 ? 0.55f : 0.9f));
        g.fillRoundedRectangle (band, 2.0f);
        g.setColour (colours::text);
        g.setFont (groupFont);
        g.drawText (modDestGroupNames()[group], band, juce::Justification::centred, true);
        d = end + 1;
    }

    g.setFont (labelFont);
    for (int d = 0; d < kNumModDests; ++d)
    {
        auto r = cellBounds (0, d);
        g.setColour (d == hoverDest ? colours::accent : colours::textDim);
        g.drawText (modDestShortNames()[d], juce::Rectangle<float> (r.getX(), groupHeight, r.getWidth(), headerHeight - groupHeight),
                    juce::Justification::centred, false);
    }

    for (int s = 0; s < kNumModSources; ++s)
    {
        auto row = cellBounds (s, 0);
        auto label = juce::Rectangle<float> (0.0f, row.getY() + 1.5f, labelWidth - 4.0f, row.getHeight() - 3.0f);

        // Live source meter behind the label
        const float level = juce::jlimit (-1.0f, 1.0f, liveSources[s]);
        g.setColour (colours::lcdBack);
        g.fillRoundedRectangle (label, 2.0f);
        const float mid = label.getCentreX();
        const bool bipolar = s == srcLfo1 || s == srcLfo2 || s == srcVco3 || s == srcVelocity || s == srcKey;
        g.setColour (colours::accent.withAlpha (0.35f));
        if (bipolar)
            g.fillRect (juce::Rectangle<float> (juce::jmin (mid, mid + level * label.getWidth() * 0.5f), label.getBottom() - 3.0f,
                                                std::abs (level) * label.getWidth() * 0.5f, 3.0f));
        else
            g.fillRect (juce::Rectangle<float> (label.getX(), label.getBottom() - 3.0f, juce::jmax (0.0f, level) * label.getWidth(), 3.0f));

        g.setColour (s == hoverSource ? colours::accent : colours::text);
        g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
        g.drawText (modSourceShortNames()[s], label.reduced (5.0f, 0.0f), juce::Justification::centredLeft, false);

        for (int d = 0; d < kNumModDests; ++d)
        {
            auto cell = cellBounds (s, d).reduced (1.5f);
            const bool hovered = s == hoverSource && d == hoverDest;
            const bool groupShade = modDestGroup (d) % 2 == 1;
            g.setColour (hovered ? colours::panelEdge.brighter (0.4f) : colours::lcdBack.brighter (groupShade ? 0.07f : 0.0f));
            g.fillRoundedRectangle (cell, 2.0f);

            const float v = cells[s][d].value;
            if (std::abs (v) > 0.001f)
            {
                const auto colour = v > 0.0f ? colours::accent : colours::accentAlt;
                const float activity = juce::jlimit (0.0f, 1.0f, std::abs (v * liveSources[s]));
                g.setColour (colour.withAlpha (0.25f + 0.45f * std::abs (v) + 0.3f * activity));
                auto fill = cell.reduced (1.0f);
                g.fillRoundedRectangle (fill.withTrimmedTop (fill.getHeight() * (1.0f - (0.25f + 0.75f * std::abs (v)))), 2.0f);
                g.setColour (juce::Colours::white);
                g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
                g.drawText ((v > 0.0f ? "+" : "") + juce::String (juce::roundToInt (v * 100.0f)), cell, juce::Justification::centred, false);
            }
        }
    }

    // Footer: either a warning about the hovered route or usage help
    auto footer = juce::Rectangle<float> (0.0f, (float) getHeight() - footerHeight + 2.0f, (float) getWidth(), footerHeight - 2.0f);
    g.setFont (juce::Font (juce::FontOptions (9.5f)));
    if (footerWarning.isNotEmpty())
    {
        g.setColour (colours::accentAlt);
        g.drawText (footerWarning, footer, juce::Justification::centredLeft, true);
    }
    else
    {
        g.setColour (colours::textDim);
        g.drawText ("Drag a cell up/down to set the amount  |  Shift = fine  |  Double-click = clear  |  Right-click = menu",
                    footer, juce::Justification::centredLeft, true);
    }
}

void ModMatrixComponent::reportHover (int source, int dest)
{
    footerWarning = source >= 0 && std::abs (cells[source][dest].value) > 0.001f ? explainSilentRoute (source, dest) : juce::String();
    if (source >= 0 && footerWarning.isEmpty() && std::abs (cells[source][dest].value) <= 0.001f)
    {
        const auto why = explainSilentRoute (source, dest);
        if (why.isNotEmpty() && ! why.startsWith ("Move") && ! why.startsWith ("Press") && ! why.startsWith ("Depends"))
            footerWarning = "Note: " + why;
    }
    if (onHover && source >= 0)
        onHover (modSourceNames()[source] + " > " + modDestNames()[dest], cells[source][dest].param->getCurrentValueAsText());
    repaint();
}

void ModMatrixComponent::setCell (int source, int dest, float amount)
{
    cells[source][dest].attachment->setValueAsCompleteGesture (amount);
}

void ModMatrixComponent::showCellMenu (int s, int d)
{
    juce::PopupMenu menu;
    menu.addSectionHeader (modSourceNames()[s] + " > " + modDestNames()[d]);
    const float amounts[] { 1.0f, 0.5f, 0.25f, 0.1f, -0.1f, -0.25f, -0.5f, -1.0f };
    for (int i = 0; i < 8; ++i)
        menu.addItem (i + 1, (amounts[i] > 0 ? "+" : "") + juce::String (juce::roundToInt (amounts[i] * 100.0f)) + "%   ("
                                 + describeModAmount (d, amounts[i]) + ")");
    menu.addSeparator();
    menu.addItem (20, "Clear this cell");
    menu.addItem (21, "Clear row (" + modSourceShortNames()[s] + ")");
    menu.addItem (22, "Clear column (" + modDestNames()[d] + ")");
    menu.addItem (23, "Clear entire matrix");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [this, s, d, amounts] (int r)
    {
        if (r >= 1 && r <= 8) setCell (s, d, amounts[r - 1]);
        else if (r == 20) setCell (s, d, 0.0f);
        else if (r == 21) for (int k = 0; k < kNumModDests; ++k) setCell (s, k, 0.0f);
        else if (r == 22) for (int k = 0; k < kNumModSources; ++k) setCell (k, d, 0.0f);
        else if (r == 23) for (int a = 0; a < kNumModSources; ++a) for (int b = 0; b < kNumModDests; ++b) setCell (a, b, 0.0f);
        reportHover (s, d);
    });
}

void ModMatrixComponent::mouseDown (const juce::MouseEvent& e)
{
    int s, d;
    if (! cellAt (e.position, s, d))
    {
        dragSource = dragDest = -1;
        return;
    }
    if (e.mods.isPopupMenu())
    {
        dragSource = dragDest = -1;
        showCellMenu (s, d);
        return;
    }
    dragSource = s;
    dragDest = d;
    dragStartValue = cells[s][d].param->convertTo0to1 (cells[s][d].value);
    cells[s][d].attachment->beginGesture();
}

void ModMatrixComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (dragSource < 0)
        return;
    // 100 px of travel covers the full -100..+100 range (normalised 0..1)
    const float sensitivity = e.mods.isShiftDown() ? 0.0015f : 0.01f;
    const float v = juce::jlimit (0.0f, 1.0f, dragStartValue - (float) e.getDistanceFromDragStartY() * sensitivity);
    auto& cell = cells[dragSource][dragDest];
    cell.attachment->setValueAsPartOfGesture (cell.param->convertFrom0to1 (v));
    reportHover (dragSource, dragDest);
}

void ModMatrixComponent::mouseUp (const juce::MouseEvent&)
{
    if (dragSource >= 0)
        cells[dragSource][dragDest].attachment->endGesture();
    dragSource = dragDest = -1;
}

void ModMatrixComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    int s, d;
    if (cellAt (e.position, s, d) && ! e.mods.isPopupMenu())
    {
        setCell (s, d, 0.0f);
        reportHover (s, d);
    }
}

void ModMatrixComponent::mouseMove (const juce::MouseEvent& e)
{
    int s = -1, d = -1;
    if (! cellAt (e.position, s, d))
        s = d = -1;
    if (s != hoverSource || d != hoverDest)
    {
        hoverSource = s;
        hoverDest = d;
        if (s < 0)
            footerWarning = {};
        reportHover (s, d);
    }
}

void ModMatrixComponent::mouseExit (const juce::MouseEvent&)
{
    hoverSource = hoverDest = -1;
    footerWarning = {};
    repaint();
}
} // namespace mf
