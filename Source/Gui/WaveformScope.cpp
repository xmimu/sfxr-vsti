#include "WaveformScope.h"
#include "SfxrTheme.h"
#include "../PluginProcessor.h"

WaveformScope::WaveformScope (SfxrVstiAudioProcessor& p) : processor (p)
{
    displayBuffer.assign (displaySize, 0.0f);
    tempBuffer.resize (displaySize);
    startTimerHz (30);
}

WaveformScope::~WaveformScope()
{
    stopTimer();
}

void WaveformScope::timerCallback()
{
    const int got = processor.readScope (tempBuffer.data(), displaySize);

    if (got > 0)
    {
        for (int i = 0; i < displaySize - got; i++)
            displayBuffer[(size_t) i] = displayBuffer[(size_t) (i + got)];
        for (int i = 0; i < got; i++)
            displayBuffer[(size_t) (displaySize - got + i)] = tempBuffer[(size_t) i];
        repaint();
    }
}

void WaveformScope::paint (juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();
    const float yMid = (float) h * 0.5f;

    g.fillAll (juce::Colour (0xFF1E1E16));

    g.setColour (juce::Colour (0xFF3A3A2E));
    for (int i = 1; i < 4; i++)
        g.drawHorizontalLine (juce::roundToInt (yMid * (float) i / 2.0f), 0.0f, (float) w);

    g.setColour (juce::Colour (0xFF2E2E26));
    for (int i = 1; i < 8; i++)
        g.drawVerticalLine (juce::roundToInt ((float) w * (float) i / 8.0f), 0.0f, (float) h);

    g.setColour (juce::Colour (0xFF55554A));
    g.drawHorizontalLine (juce::roundToInt (yMid), 0.0f, (float) w);

    g.setColour (SfxrTheme::kBarFill);
    juce::Path path;
    const int n = displaySize;
    const float scaleY = yMid * 0.9f;
    for (int i = 0; i < n; i++)
    {
        const float x = (float) i / (float) (n - 1) * (float) w;
        const float y = yMid - displayBuffer[(size_t) i] * scaleY;
        if (i == 0) path.startNewSubPath (x, y);
        else        path.lineTo (x, y);
    }
    g.strokePath (path, juce::PathStrokeType (1.0f));
}
