#include "ModMatrixComponent.h"
#include "ForgeLookAndFeel.h"

namespace mf
{
ModMatrixComponent::ModMatrixComponent (juce::AudioProcessorValueTreeState& apvts)
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
}

juce::Rectangle<float> ModMatrixComponent::cellBounds (int source, int dest) const
{
    const float cw = ((float) getWidth() - labelWidth) / (float) kNumModDests;
    const float ch = ((float) getHeight() - headerHeight) / (float) kNumModSources;
    return { labelWidth + (float) dest * cw, headerHeight + (float) source * ch, cw, ch };
}

bool ModMatrixComponent::cellAt (juce::Point<float> p, int& source, int& dest) const
{
    if (p.x < labelWidth || p.y < headerHeight)
        return false;
    const float cw = ((float) getWidth() - labelWidth) / (float) kNumModDests;
    const float ch = ((float) getHeight() - headerHeight) / (float) kNumModSources;
    dest = juce::jlimit (0, kNumModDests - 1, (int) ((p.x - labelWidth) / cw));
    source = juce::jlimit (0, kNumModSources - 1, (int) ((p.y - headerHeight) / ch));
    return true;
}

void ModMatrixComponent::paint (juce::Graphics& g)
{
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));

    for (int d = 0; d < kNumModDests; ++d)
    {
        auto r = cellBounds (0, d);
        g.setColour (d == hoverDest ? colours::accent : colours::textDim);
        g.drawText (modDestShortNames()[d], juce::Rectangle<float> (r.getX(), 0.0f, r.getWidth(), headerHeight), juce::Justification::centred);
    }

    for (int s = 0; s < kNumModSources; ++s)
    {
        auto r = cellBounds (s, 0);
        g.setColour (s == hoverSource ? colours::accent : colours::textDim);
        g.drawText (modSourceShortNames()[s], juce::Rectangle<float> (0.0f, r.getY(), labelWidth - 4.0f, r.getHeight()), juce::Justification::centredRight);

        for (int d = 0; d < kNumModDests; ++d)
        {
            auto cell = cellBounds (s, d).reduced (1.5f);
            const bool hovered = s == hoverSource && d == hoverDest;
            g.setColour (hovered ? colours::panelEdge.brighter (0.4f) : (((s + d) % 2) ? colours::lcdBack : colours::lcdBack.brighter (0.06f)));
            g.fillRoundedRectangle (cell, 2.0f);

            const float v = cells[s][d].value;
            if (std::abs (v) > 0.001f)
            {
                const auto colour = v > 0.0f ? colours::accent : colours::accentAlt;
                const float size = juce::jmin (cell.getWidth(), cell.getHeight()) * (0.3f + 0.7f * std::sqrt (std::abs (v)));
                g.setColour (colour.withAlpha (0.25f + 0.75f * std::abs (v)));
                g.fillEllipse (cell.withSizeKeepingCentre (size, size));
            }
        }
    }
}

void ModMatrixComponent::reportHover (int source, int dest)
{
    if (onHover && source >= 0)
        onHover (modSourceNames()[source] + " > " + modDestNames()[dest], cells[source][dest].param->getCurrentValueAsText());
}

void ModMatrixComponent::mouseDown (const juce::MouseEvent& e)
{
    if (! cellAt (e.position, dragSource, dragDest))
    {
        dragSource = dragDest = -1;
        return;
    }
    dragStartValue = cells[dragSource][dragDest].param->convertTo0to1 (cells[dragSource][dragDest].value);
    cells[dragSource][dragDest].attachment->beginGesture();
}

void ModMatrixComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (dragSource < 0)
        return;
    const float sensitivity = e.mods.isShiftDown() ? 0.0008f : 0.005f;
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
    if (cellAt (e.position, s, d))
    {
        cells[s][d].attachment->setValueAsCompleteGesture (0.0f);
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
        reportHover (s, d);
        repaint();
    }
}

void ModMatrixComponent::mouseExit (const juce::MouseEvent&)
{
    hoverSource = hoverDest = -1;
    repaint();
}
} // namespace mf
