#include "SfxrLookAndFeel.h"
#include "SfxrTheme.h"

SfxrLookAndFeel::SfxrLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, SfxrTheme::kBg);
    setColour (juce::TextButton::buttonColourId, SfxrTheme::kButtonBg);
    setColour (juce::TextButton::buttonOnColourId, SfxrTheme::kButtonHover);
    setColour (juce::TextButton::textColourOffId, SfxrTheme::kTextDark);
    setColour (juce::TextButton::textColourOnId, SfxrTheme::kTextDark);
    setColour (juce::ToggleButton::textColourId, SfxrTheme::kTextDark);
    setColour (juce::ToggleButton::tickColourId, SfxrTheme::kBarFill);
    setColour (juce::ToggleButton::tickDisabledColourId, SfxrTheme::kButtonBg);
    setColour (juce::Label::textColourId, SfxrTheme::kTextDark);
    setColour (juce::ComboBox::backgroundColourId, SfxrTheme::kButtonBg);
    setColour (juce::ComboBox::textColourId, SfxrTheme::kTextDark);
    setColour (juce::ComboBox::outlineColourId, SfxrTheme::kDivider);
    setColour (juce::ComboBox::arrowColourId, SfxrTheme::kTextDark);
    setColour (juce::ComboBox::focusedOutlineColourId, SfxrTheme::kTextDark);
    setColour (juce::TextEditor::backgroundColourId, SfxrTheme::kButtonBg);
    setColour (juce::TextEditor::textColourId, SfxrTheme::kTextDark);
    setColour (juce::TextEditor::outlineColourId, SfxrTheme::kDivider);
    setColour (juce::TextEditor::focusedOutlineColourId, SfxrTheme::kTextDark);
    setColour (juce::PopupMenu::backgroundColourId, SfxrTheme::kBg);
    setColour (juce::PopupMenu::textColourId, SfxrTheme::kTextDark);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, SfxrTheme::kBarFill);
    setColour (juce::PopupMenu::highlightedTextColourId, SfxrTheme::kTextDark);
    setColour (juce::AlertWindow::backgroundColourId, SfxrTheme::kBg);
    setColour (juce::AlertWindow::textColourId, SfxrTheme::kTextDark);
    setColour (juce::AlertWindow::outlineColourId, SfxrTheme::kDivider);
}

void SfxrLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        const juce::Slider::SliderStyle, juce::Slider& slider)
{
    const float trackY  = (float) y + (float) height * 0.5f - 4.0f;
    const float trackH  = 8.0f;
    const float left    = (float) x;
    const float right   = (float) (x + width);

    g.setColour (SfxrTheme::kBarBg);
    g.fillRect (left, trackY, right - left, trackH);

    g.setColour (SfxrTheme::kBarFill);
    g.fillRect (left, trackY, sliderPos - left, trackH);

    g.setColour (juce::Colours::white);
    g.fillRect (sliderPos - 1.0f, trackY, 2.0f, trackH);

    if (slider.getMinimum() < 0.0f)
    {
        const float centre = (maxSliderPos + minSliderPos) * 0.5f;
        g.setColour (SfxrTheme::kDivider);
        g.fillRect (centre - 0.5f, (float) y, 1.0f, 3.0f);
        g.fillRect (centre - 0.5f, (float) (y + height - 3), 1.0f, 3.0f);
    }
}

void SfxrLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                    int, int, int buttonW, int, juce::ComboBox&)
{
    g.fillAll (isButtonDown ? SfxrTheme::kButtonHover : SfxrTheme::kButtonBg);
    g.setColour (SfxrTheme::kDivider);
    g.drawRect (0, 0, width, height, 1);
    g.drawLine ((float) (width - buttonW), 1.0f, (float) (width - buttonW),
                (float) (height - 1), 1.0f);

    const float centreX = (float) width - (float) buttonW * 0.5f;
    const float centreY = (float) height * 0.5f;
    juce::Path arrow;
    arrow.addTriangle (centreX - 4.0f, centreY - 2.0f, centreX + 4.0f, centreY - 2.0f,
                       centreX, centreY + 3.0f);
    g.fillPath (arrow);
}
