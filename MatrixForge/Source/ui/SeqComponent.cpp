#include "SeqComponent.h"
#include "ForgeLookAndFeel.h"

namespace mf
{
SeqComponent::SeqComponent (juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 0; i < kNumSeqSteps; ++i)
    {
        auto& lane = lanes[i];
        lane.step = apvts.getParameter (seqStepId (i));
        lane.gate = apvts.getParameter (seqGateId (i));
        lane.stepAttachment = std::make_unique<juce::ParameterAttachment> (*lane.step, [this, &lane] (float v) { lane.stepValue = v; repaint(); });
        lane.gateAttachment = std::make_unique<juce::ParameterAttachment> (*lane.gate, [this, &lane] (float v) { lane.gateValue = v; repaint(); });
        lane.stepAttachment->sendInitialUpdate();
        lane.gateAttachment->sendInitialUpdate();
    }
    lengthParam = apvts.getParameter ("seqLength");
    lengthAttachment = std::make_unique<juce::ParameterAttachment> (*lengthParam, [this] (float v) { length = (int) v; repaint(); });
    lengthAttachment->sendInitialUpdate();
}

void SeqComponent::setPlayingStep (int step)
{
    if (step != playingStep)
    {
        playingStep = step;
        repaint();
    }
}

int SeqComponent::laneAt (float x) const
{
    return juce::jlimit (0, kNumSeqSteps - 1, (int) (x / ((float) getWidth() / (float) kNumSeqSteps)));
}

void SeqComponent::paint (juce::Graphics& g)
{
    const float laneW = (float) getWidth() / (float) kNumSeqSteps;
    const float laneH = gateStripTop() - 4.0f;
    const float zeroY = laneH * 0.5f;

    for (int i = 0; i < kNumSeqSteps; ++i)
    {
        const auto& lane = lanes[i];
        const bool inRange = i < length;
        auto r = juce::Rectangle<float> ((float) i * laneW + 1.5f, 0.0f, laneW - 3.0f, laneH);

        g.setColour (i == playingStep ? colours::accent.withAlpha (0.25f) : colours::lcdBack.brighter ((i / 4) % 2 ? 0.0f : 0.05f));
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (colours::panelEdge);
        g.drawHorizontalLine ((int) zeroY, r.getX(), r.getRight());

        const int semis = juce::roundToInt (lane.stepValue);
        const float y = zeroY - (float) semis / 24.0f * (laneH * 0.5f - 2.0f);
        const bool gateOn = lane.gateValue >= 0.5f;
        auto colour = gateOn ? colours::accent : colours::textDim;
        if (! inRange)
            colour = colour.withAlpha (0.25f);
        g.setColour (colour.withAlpha (inRange ? 0.6f : 0.15f));
        g.fillRect (juce::Rectangle<float> (r.getX() + 3.0f, juce::jmin (y, zeroY), r.getWidth() - 6.0f, std::abs (zeroY - y)));
        g.setColour (colour);
        g.fillRect (r.getX() + 2.0f, y - 1.5f, r.getWidth() - 4.0f, 3.0f);

        g.setFont (juce::Font (juce::FontOptions (9.0f)));
        g.setColour (colours::text.withAlpha (inRange ? 0.8f : 0.3f));
        g.drawText (juce::String (semis), r.removeFromBottom (12.0f), juce::Justification::centred);

        auto gateRect = juce::Rectangle<float> ((float) i * laneW + 3.0f, gateStripTop(), laneW - 6.0f, 12.0f);
        g.setColour (gateOn && inRange ? colours::accent : colours::panelEdge);
        g.fillRoundedRectangle (gateRect, 2.0f);
    }
}

void SeqComponent::setFromY (int lane, float y)
{
    const float laneH = gateStripTop() - 4.0f;
    const float semis = juce::jlimit (-24.0f, 24.0f, std::round ((laneH * 0.5f - y) / (laneH * 0.5f - 2.0f) * 24.0f));
    lanes[lane].stepAttachment->setValueAsPartOfGesture (semis);
    if (onHover)
        onHover (lanes[lane].step->getName (64), lanes[lane].step->getCurrentValueAsText());
}

void SeqComponent::mouseDown (const juce::MouseEvent& e)
{
    const int lane = laneAt (e.position.x);
    if (e.position.y >= gateStripTop() - 2.0f)
    {
        lanes[lane].gateAttachment->setValueAsCompleteGesture (lanes[lane].gateValue >= 0.5f ? 0.0f : 1.0f);
        dragLane = -1;
        return;
    }
    dragLane = lane;
    lanes[lane].stepAttachment->beginGesture();
    setFromY (lane, e.position.y);
}

void SeqComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (dragLane < 0)
        return;
    const int lane = laneAt (e.position.x);
    if (lane != dragLane) // paint across lanes
    {
        lanes[dragLane].stepAttachment->endGesture();
        dragLane = lane;
        lanes[dragLane].stepAttachment->beginGesture();
    }
    setFromY (dragLane, e.position.y);
}

void SeqComponent::mouseUp (const juce::MouseEvent&)
{
    if (dragLane >= 0)
        lanes[dragLane].stepAttachment->endGesture();
    dragLane = -1;
}

void SeqComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (e.position.y < gateStripTop() - 2.0f)
        lanes[laneAt (e.position.x)].stepAttachment->setValueAsCompleteGesture (0.0f);
}
} // namespace mf
