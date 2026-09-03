#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SfxrEngine/SfxrAudioExporter.h"

namespace
{
    // sfxr palette
    const juce::Colour kBg          (0xFFC0B090);
    const juce::Colour kTextDark    (0xFF504030);
    const juce::Colour kBarBg       (0xFF807060);
    const juce::Colour kBarFill     (0xFFF0C090);
    const juce::Colour kButtonBg    (0xFFE0D0B0);
    const juce::Colour kButtonHover (0xFFFFF0E0);
    const juce::Colour kDivider     (0xFF000000);

    class WaveformScopeComponent : public juce::Component, private juce::Timer
    {
    public:
        explicit WaveformScopeComponent (SfxrVstiAudioProcessor& p) : processor (p)
        {
            displayBuffer.assign (displaySize, 0.0f);
            tempBuffer.resize (displaySize);
            startTimerHz (30);
        }

        ~WaveformScopeComponent() override { stopTimer(); }

        void timerCallback() override
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

        void paint (juce::Graphics& g) override
        {
            const int w = getWidth();
            const int h = getHeight();
            const float yMid = h * 0.5f;

            g.fillAll (juce::Colour (0xFF1E1E16));

            g.setColour (juce::Colour (0xFF3A3A2E));
            for (int i = 1; i < 4; i++)
                g.drawHorizontalLine (juce::roundToInt (yMid * i / 2.0f), 0.0f, (float) w);

            g.setColour (juce::Colour (0xFF2E2E26));
            for (int i = 1; i < 8; i++)
                g.drawVerticalLine (juce::roundToInt (w * i / 8.0f), 0.0f, (float) h);

            g.setColour (juce::Colour (0xFF55554A));
            g.drawHorizontalLine (juce::roundToInt (yMid), 0.0f, (float) w);

            g.setColour (kBarFill);
            juce::Path path;
            const int n = displaySize;
            const float scaleY = yMid * 0.9f;
            for (int i = 0; i < n; i++)
            {
                const float x = (float) i / (float) (n - 1) * (float) w;
                const float y = yMid - displayBuffer[(size_t) i] * scaleY;
                if (i == 0) path.startNewSubPath (x, y);
                else         path.lineTo (x, y);
            }
            g.strokePath (path, juce::PathStrokeType (1.0f));
        }

    private:
        static constexpr int displaySize = 8192;
        SfxrVstiAudioProcessor& processor;
        std::vector<float> displayBuffer;
        std::vector<float> tempBuffer;
    };

    class SfxrLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        SfxrLookAndFeel()
        {
            setColour (juce::ResizableWindow::backgroundColourId, kBg);
            setColour (juce::TextButton::buttonColourId, kButtonBg);
            setColour (juce::TextButton::buttonOnColourId, kButtonHover);
            setColour (juce::TextButton::textColourOffId, kTextDark);
            setColour (juce::TextButton::textColourOnId, kTextDark);
            setColour (juce::ToggleButton::textColourId, kTextDark);
            setColour (juce::ToggleButton::tickColourId, kBarFill);
            setColour (juce::ToggleButton::tickDisabledColourId, kButtonBg);
            setColour (juce::Label::textColourId, kTextDark);
            setColour (juce::ComboBox::backgroundColourId, kButtonBg);
            setColour (juce::ComboBox::textColourId, kTextDark);
            setColour (juce::ComboBox::outlineColourId, kDivider);
            setColour (juce::ComboBox::arrowColourId, kTextDark);
            setColour (juce::ComboBox::focusedOutlineColourId, kTextDark);
            setColour (juce::TextEditor::backgroundColourId, kButtonBg);
            setColour (juce::TextEditor::textColourId, kTextDark);
            setColour (juce::TextEditor::outlineColourId, kDivider);
            setColour (juce::TextEditor::focusedOutlineColourId, kTextDark);
            setColour (juce::PopupMenu::backgroundColourId, kBg);
            setColour (juce::PopupMenu::textColourId, kTextDark);
            setColour (juce::PopupMenu::highlightedBackgroundColourId, kBarFill);
            setColour (juce::PopupMenu::highlightedTextColourId, kTextDark);
            setColour (juce::AlertWindow::backgroundColourId, kBg);
            setColour (juce::AlertWindow::textColourId, kTextDark);
            setColour (juce::AlertWindow::outlineColourId, kDivider);
        }

        void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               const juce::Slider::SliderStyle, juce::Slider& slider) override
        {
            const float trackY  = y + height * 0.5f - 4.0f;
            const float trackH  = 8.0f;
            const float left    = (float) x;
            const float right   = (float) (x + width);

            g.setColour (kBarBg);
            g.fillRect (left, trackY, right - left, trackH);

            g.setColour (kBarFill);
            g.fillRect (left, trackY, sliderPos - left, trackH);

            g.setColour (juce::Colours::white);
            g.fillRect (sliderPos - 1.0f, trackY, 2.0f, trackH);

            if (slider.getMinimum() < 0.0f)
            {
                const float centre = (maxSliderPos + minSliderPos) * 0.5f;
                g.setColour (kDivider);
                g.fillRect (centre - 0.5f, (float) y, 1.0f, 3.0f);
                g.fillRect (centre - 0.5f, (float) (y + height - 3), 1.0f, 3.0f);
            }
        }

        void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                           int, int, int buttonW, int, juce::ComboBox&) override
        {
            g.fillAll (isButtonDown ? kButtonHover : kButtonBg);
            g.setColour (kDivider);
            g.drawRect (0, 0, width, height, 1);
            g.drawLine ((float) (width - buttonW), 1.0f, (float) (width - buttonW),
                        (float) (height - 1), 1.0f);

            const float centreX = width - buttonW * 0.5f;
            const float centreY = height * 0.5f;
            juce::Path arrow;
            arrow.addTriangle (centreX - 4.0f, centreY - 2.0f, centreX + 4.0f, centreY - 2.0f,
                               centreX, centreY + 3.0f);
            g.fillPath (arrow);
        }
    };

    // AlertWindow's static show*() helpers always use the global default LookAndFeel,
    // which would look out of place next to the rest of this sfxr-styled UI. Building
    // the window directly lets us apply the same LookAndFeel as everything else.
    void showStyledAlert (juce::LookAndFeel& lf, juce::MessageBoxIconType icon,
                          const juce::String& title, const juce::String& message,
                          juce::Component* associatedComponent)
    {
        auto* alert = new juce::AlertWindow (title, message, icon, associatedComponent);
        alert->setLookAndFeel (&lf);
        alert->addButton ("OK", 0, juce::KeyPress (juce::KeyPress::returnKey));
        alert->enterModalState (true, juce::ModalCallbackFunction::create ([alert] (int)
        {
            std::unique_ptr<juce::AlertWindow> deleter (alert);
        }), false);
    }

    void showStyledConfirm (juce::LookAndFeel& lf, juce::MessageBoxIconType icon,
                            const juce::String& title, const juce::String& message,
                            juce::Component* associatedComponent, std::function<void()> onConfirm)
    {
        auto* alert = new juce::AlertWindow (title, message, icon, associatedComponent);
        alert->setLookAndFeel (&lf);
        alert->addButton ("Replace", 1, juce::KeyPress (juce::KeyPress::returnKey));
        alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        alert->enterModalState (true, juce::ModalCallbackFunction::create ([alert, onConfirm] (int result)
        {
            std::unique_ptr<juce::AlertWindow> deleter (alert);
            if (result == 1)
                onConfirm();
        }), false);
    }

    // Remembers the last export choices (format/rate/encoding/folder) between dialog opens.
    juce::PropertiesFile::Options getExportSettingsOptions()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "SfxrVsti";
        options.filenameSuffix = "settings";
        options.folderName = "SfxrVsti";
        options.osxLibrarySubFolder = "Application Support";
        return options;
    }

    class ExportOptionsComponent : public juce::Component
    {
    public:
        using ExportCallback = std::function<void (int, int, int, const juce::File&)>;

        explicit ExportOptionsComponent (ExportCallback callback)
            : onExport (std::move (callback))
        {
            juce::PropertiesFile settings (getExportSettingsOptions());
            outputDirectory = juce::File (settings.getValue ("exportDirectory"));
            if (! outputDirectory.isDirectory())
                outputDirectory = juce::File::getSpecialLocation (juce::File::userHomeDirectory);

            format.addItem ("WAV", 1);
            format.addItem ("OGG", 2);
            format.setSelectedId (settings.getIntValue ("exportFormat", 1));
            format.onChange = [this] { updateEncodingChoices(); };

            for (const auto& rate : { "44.1 kHz", "48 kHz", "88.2 kHz", "96 kHz", "192 kHz" })
                sampleRate.addItem (rate, sampleRate.getNumItems() + 1);
            sampleRate.setSelectedId (settings.getIntValue ("exportSampleRate", 1));

            fileName.setText ("sfxr-sound", false);
            fileName.setSelectAllWhenFocused (true);

            chooseFolder.setButtonText ("CHOOSE FOLDER");
            chooseFolder.onClick = [this] { chooseOutputDirectory(); };
            cancel.setButtonText ("CANCEL");
            cancel.onClick = [this] { closeDialog(); };
            exportButton.setButtonText ("EXPORT");
            exportButton.onClick = [this] { exportFile(); };

            for (auto* component : std::initializer_list<juce::Component*> {
                     &format, &sampleRate, &encoding, &fileName, &chooseFolder, &cancel, &exportButton })
                addAndMakeVisible (component);

            updateEncodingChoices();
            encoding.setSelectedId (settings.getIntValue ("exportEncoding", encoding.getSelectedId()));
            setSize (380, 300);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (kBg);
            g.setColour (kTextDark);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText ("FORMAT", 16, 12, 160, 16, juce::Justification::centredLeft);
            g.drawText ("SAMPLE RATE", 16, 58, 160, 16, juce::Justification::centredLeft);
            g.drawText ("BIT DEPTH / BITRATE", 16, 104, 180, 16, juce::Justification::centredLeft);
            g.drawText ("EXPORT FOLDER", 16, 150, 180, 16, juce::Justification::centredLeft);
            g.drawText ("FILE NAME", 16, 200, 160, 16, juce::Justification::centredLeft);
            g.setFont (11.0f);
            g.drawText (outputDirectory.getFullPathName(), 16, 168, 226, 20, juce::Justification::centredLeft, true);
        }

        void resized() override
        {
            constexpr int left = 16;
            constexpr int width = 348;
            format.setBounds (left, 28, width, 22);
            sampleRate.setBounds (left, 74, width, 22);
            encoding.setBounds (left, 120, width, 22);
            chooseFolder.setBounds (248, 166, 116, 24);
            fileName.setBounds (left, 216, width, 24);
            cancel.setBounds (100, 258, 80, 24);
            exportButton.setBounds (200, 258, 80, 24);
        }

    private:
        void updateEncodingChoices()
        {
            encoding.clear();
            if (format.getSelectedId() == 1)
            {
                encoding.addItem ("16-bit PCM", 16);
                encoding.addItem ("24-bit PCM", 24);
                encoding.addItem ("32-bit float", 32);
                encoding.setSelectedId (24);
            }
            else
            {
                const auto qualities = juce::OggVorbisAudioFormat().getQualityOptions();
                for (int i = 0; i < qualities.size(); ++i)
                    encoding.addItem (qualities[i], i + 1);
                encoding.setSelectedId (5);
            }
        }

        void chooseOutputDirectory()
        {
            auto chooser = std::make_shared<juce::FileChooser> ("Choose export folder", outputDirectory);
            juce::Component::SafePointer<ExportOptionsComponent> component (this);
            chooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectDirectories,
                                  [component, chooser] (const juce::FileChooser& fileChooser)
            {
                if (component != nullptr && fileChooser.getResult().isDirectory())
                {
                    component->outputDirectory = fileChooser.getResult();
                    component->repaint (0, 166, 230, 24);
                }
            });
        }

        void exportFile()
        {
            const auto name = fileName.getText().trim();
            if (name.isEmpty())
            {
                fileName.grabKeyboardFocus();
                return;
            }

            const auto extension = format.getSelectedId() == 1 ? ".wav" : ".ogg";
            const auto target = outputDirectory.getChildFile (name).withFileExtension (extension);

            if (! target.existsAsFile())
            {
                performExport (target);
                return;
            }

            juce::Component::SafePointer<ExportOptionsComponent> component (this);
            showStyledConfirm (getLookAndFeel(), juce::MessageBoxIconType::WarningIcon,
                               "File Already Exists",
                               "\"" + target.getFileName() + "\" already exists. Replace it?",
                               this, [component, target]
            {
                if (component != nullptr)
                    component->performExport (target);
            });
        }

        void performExport (const juce::File& target)
        {
            juce::PropertiesFile settings (getExportSettingsOptions());
            settings.setValue ("exportFormat", format.getSelectedId());
            settings.setValue ("exportSampleRate", sampleRate.getSelectedId());
            settings.setValue ("exportEncoding", encoding.getSelectedId());
            settings.setValue ("exportDirectory", outputDirectory.getFullPathName());

            onExport (format.getSelectedId(), sampleRate.getSelectedId(), encoding.getSelectedId(), target);
            closeDialog();
        }

        void closeDialog()
        {
            if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
                dialog->exitModalState (0);
        }

        ExportCallback onExport;
        juce::File outputDirectory;
        juce::ComboBox format, sampleRate, encoding;
        juce::TextEditor fileName;
        juce::TextButton chooseFolder, cancel, exportButton;
    };

    // A MidiKeyboardComponent that shows the full 88-key range, highlights the
    // root note and greys out + disables every key outside the valid range.
    class SfxrMidiKeyboardComponent : public juce::MidiKeyboardComponent
    {
    public:
        // Note 69 plays the Start Frequency parameter unmodified, so it is the
        // root of the transposition -- not a 440 Hz concert A. Labelling it "A4"
        // would imply a pitch the synth does not actually produce.
        static constexpr int rootNote = 69;

        using juce::MidiKeyboardComponent::MidiKeyboardComponent;

        bool isNoteEnabled (int midiNoteNumber) const noexcept
        {
            return SfxrNoteRange::contains (midiNoteNumber);
        }

        void drawWhiteNote (int midiNoteNumber, juce::Graphics& g, juce::Rectangle<float> area,
                            bool isDown, bool isOver, juce::Colour lineColour, juce::Colour textColour) override
        {
            if (! isNoteEnabled (midiNoteNumber))
            {
                g.setColour (disabledWhiteColour);
                g.fillRect (area);
                juce::MidiKeyboardComponent::drawWhiteNote (midiNoteNumber, g, area, false, false, lineColour, textColour);
                return;
            }

            if (midiNoteNumber == rootNote)
            {
                auto c = rootNoteColour;
                if (isDown) c = c.overlaidWith (findColour (keyDownOverlayColourId));
                if (isOver) c = c.overlaidWith (findColour (mouseOverKeyOverlayColourId));

                g.setColour (c);
                g.fillRect (area);
            }

            juce::MidiKeyboardComponent::drawWhiteNote (midiNoteNumber, g, area, isDown, isOver, lineColour, textColour);
        }

        void drawBlackNote (int midiNoteNumber, juce::Graphics& g, juce::Rectangle<float> area,
                            bool isDown, bool isOver, juce::Colour noteFillColour) override
        {
            if (! isNoteEnabled (midiNoteNumber))
            {
                g.setColour (disabledBlackColour);
                g.fillRect (area);

                const auto sideIndent = 1.0f / 8.0f;
                const auto topIndent  = 7.0f / 8.0f;
                g.setColour (disabledBlackColour.brighter (0.15f));
                g.fillRect (area.reduced (area.getWidth() * sideIndent, 0.0f)
                                 .removeFromTop (area.getHeight() * topIndent));
                return;
            }

            juce::MidiKeyboardComponent::drawBlackNote (midiNoteNumber, g, area, isDown, isOver, noteFillColour);
        }

        bool mouseDownOnKey (int midiNoteNumber, const juce::MouseEvent& e) override
        {
            return isNoteEnabled (midiNoteNumber)
                && juce::MidiKeyboardComponent::mouseDownOnKey (midiNoteNumber, e);
        }

        bool mouseDraggedToKey (int midiNoteNumber, const juce::MouseEvent& e) override
        {
            return isNoteEnabled (midiNoteNumber)
                && juce::MidiKeyboardComponent::mouseDraggedToKey (midiNoteNumber, e);
        }

        void mouseUpOnKey (int midiNoteNumber, const juce::MouseEvent& e) override
        {
            if (isNoteEnabled (midiNoteNumber))
                juce::MidiKeyboardComponent::mouseUpOnKey (midiNoteNumber, e);
        }

        juce::String getWhiteNoteText (int midiNoteNumber) override
        {
            if (midiNoteNumber == rootNote)
                return "ROOT";

            return juce::MidiKeyboardComponent::getWhiteNoteText (midiNoteNumber);
        }

    private:
        const juce::Colour rootNoteColour      { 0xFFE8A030 };
        const juce::Colour disabledWhiteColour { 0xFFC8C0B8 };
        const juce::Colour disabledBlackColour { 0xFF908880 };
    };
}

SfxrVstiAudioProcessorEditor::SfxrVstiAudioProcessorEditor (SfxrVstiAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      apvts (p.getAPVTS())
{
    lookAndFeel = std::make_unique<SfxrLookAndFeel>();
    setLookAndFeel (lookAndFeel.get());

    apvts.addParameterListener (juce::String (ParamID::wave_type), this);

    buildInterface();

    setResizable (false, false);
    setSize (880, 700);
}

SfxrVstiAudioProcessorEditor::~SfxrVstiAudioProcessorEditor()
{
    apvts.removeParameterListener (juce::String (ParamID::wave_type), this);
    setLookAndFeel (nullptr);
}

void SfxrVstiAudioProcessorEditor::parentHierarchyChanged()
{
    if (configuredStandaloneTitleBar || ! juce::JUCEApplicationBase::isStandaloneApp())
        return;

    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
    {
        window->setUsingNativeTitleBar (true);
        window->setContentComponentSize (880, 700);
        configuredStandaloneTitleBar = true;
    }
}

void SfxrVstiAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.setColour (kTextDark);
    g.drawText ("sfxr", 12, 6, 60, 24, juce::Justification::centredLeft);

    g.setFont (12.0f);
    g.drawText ("VSTi", 12, 26, 60, 16, juce::Justification::centredLeft);
    g.drawText (juce::String ("v") + JucePlugin_VersionString,
                62, 26, 58, 16, juce::Justification::centredRight);

    g.setColour (kDivider);
    g.drawLine (128.0f, 36.0f, 128.0f, 700.0f);
    g.drawLine (12.0f, 234.0f, 122.0f, 234.0f);
    g.drawLine (12.0f, 318.0f, 122.0f, 318.0f);

    // divider above the waveform display
    g.setColour (kDivider);
    g.drawLine (8.0f, 480.0f, 872.0f, 480.0f);

    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("WAVEFORM", 16, 466, 120, 14, juce::Justification::centredLeft);
    g.drawText ("MIDI KEYBOARD", 16, 590, 160, 14, juce::Justification::centredLeft);
}

void SfxrVstiAudioProcessorEditor::parameterChanged (const juce::String& id, float)
{
    if (id == ParamID::wave_type)
        updateWaveButtons();
}

void SfxrVstiAudioProcessorEditor::addSectionHeader (const juce::String& text, int x, int y)
{
    auto* l = own (new juce::Label());
    l->setText (text, juce::dontSendNotification);
    l->setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    l->setColour (juce::Label::textColourId, kTextDark);
    l->setBounds (x, y, 220, 18);
}

// The look and feel decides whether to draw a bipolar centre marker from the
// slider's own range, so no extra flag is needed here.
juce::Slider* SfxrVstiAudioProcessorEditor::addSlider (const juce::String& paramID,
                                                       const juce::String& label,
                                                       int x, int y, int barWidth)
{
    constexpr int labelWidth = 96;
    constexpr int rowHeight  = 18;

    auto* lbl = own (new juce::Label());
    lbl->setText (label, juce::dontSendNotification);
    lbl->setFont (juce::Font (juce::FontOptions (11.0f)));
    lbl->setColour (juce::Label::textColourId, kTextDark);
    lbl->setBounds (x, y + 1, labelWidth, rowHeight);
    lbl->setJustificationType (juce::Justification::centredLeft);

    auto* s = own (new juce::Slider());
    s->setSliderStyle (juce::Slider::LinearHorizontal);
    s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s->setBounds (x + labelWidth, y, barWidth, rowHeight);

    sliderAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramID, *s));

    return s;
}

void SfxrVstiAudioProcessorEditor::addWaveButton (const juce::String& text, int value, int x, int y)
{
    auto* b = own (new juce::ToggleButton (text));
    b->setColour (juce::ToggleButton::tickColourId, kBarFill);
    b->setColour (juce::ToggleButton::tickDisabledColourId, kButtonBg);
    b->setBounds (x, y, 104, 22);
    b->setRadioGroupId (1001);
    b->setClickingTogglesState (true);
    b->onClick = [this, value]
    {
        if (auto* p = apvts.getParameter (ParamID::wave_type))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) value));
            p->endChangeGesture();
        }
    };

    if (value >= 0 && value < 4)
        waveButtons[value] = b;
}

void SfxrVstiAudioProcessorEditor::updateWaveButtons()
{
    const int wave = juce::roundToInt (apvts.getRawParameterValue (ParamID::wave_type)->load());
    for (int i = 0; i < 4; i++)
        if (waveButtons[i] != nullptr)
            waveButtons[i]->setToggleState (i == wave, juce::dontSendNotification);
}

void SfxrVstiAudioProcessorEditor::buildInterface()
{
    // ---- waveform selector (top) ----
    addWaveButton ("SQUAREWAVE", 0, 150, 8);
    addWaveButton ("SAWTOOTH",   1, 256, 8);
    addWaveButton ("SINEWAVE",   2, 362, 8);
    addWaveButton ("NOISE",      3, 468, 8);

    auto* monoButton = own (new juce::ToggleButton ("MONO"));
    monoButton->setBounds (580, 8, 80, 22);
    buttonAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParamID::mono, *monoButton));

    auto* oneShotButton = own (new juce::ToggleButton ("ONE-SHOT"));
    oneShotButton->setBounds (668, 8, 90, 22);
    buttonAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParamID::one_shot, *oneShotButton));

    // ---- generator column (left) ----
    addSectionHeader ("GENERATOR", 12, 40);

    int gy = 62;
    for (int i = 0; i < (int) PresetCategory::Count; i++)
    {
        const auto cat = (PresetCategory) i;
        auto* b = own (new juce::TextButton (presetCategoryName (cat)));
        b->setBounds (12, gy, 110, 22);
        b->onClick = [this, cat] { generateCategory (cat); };
        gy += 24;
    }

    gy += 12;

    auto addActionButton = [this, &gy] (const juce::String& text, std::function<void()> fn)
    {
        auto* b = own (new juce::TextButton (text));
        b->setBounds (12, gy, 110, 22);
        b->onClick = std::move (fn);
        gy += 24;
    };

    addActionButton ("MUTATE",      [this] { mutate(); });
    addActionButton ("RANDOMIZE",   [this] { randomize(); });
    addActionButton ("PLAY SOUND",  [this] { audioProcessor.playPreview(); });

    gy += 12;

    addActionButton ("LOAD CONFIG", [this] { loadSfs(); });
    addActionButton ("SAVE CONFIG", [this] { saveSfs(); });

    if (juce::JUCEApplicationBase::isStandaloneApp())
        addActionButton ("EXPORT AUDIO", [this] { exportAudio(); });

    // ---- manual settings (right) ----
    addSectionHeader ("MANUAL SETTINGS", 140, 40);

    addSectionHeader ("ENVELOPE", 140, 68);
    addSlider (ParamID::env_attack,  "ATTACK TIME",   140, 88, 130);
    addSlider (ParamID::env_sustain, "SUSTAIN TIME",  140, 108, 130);
    addSlider (ParamID::env_punch,   "SUSTAIN PUNCH", 140, 128, 130);
    addSlider (ParamID::env_decay,   "DECAY TIME",    140, 148, 130);

    addSectionHeader ("VIBRATO", 140, 176);
    addSlider (ParamID::vib_strength, "DEPTH", 140, 196, 130);
    addSlider (ParamID::vib_speed,    "SPEED", 140, 216, 130);
    addSlider (ParamID::vib_delay,    "DELAY", 140, 236, 130);

    addSectionHeader ("FREQUENCY", 384, 68);
    addSlider (ParamID::base_freq,  "START FREQ",  384, 88, 130);
    addSlider (ParamID::freq_limit, "MIN FREQ",    384, 108, 130);
    addSlider (ParamID::freq_ramp,  "SLIDE",       384, 128, 130);
    addSlider (ParamID::freq_dramp, "DELTA SLIDE", 384, 148, 130);

    addSectionHeader ("SQUARE DUTY", 384, 176);
    addSlider (ParamID::duty,      "DUTY",      384, 196, 130);
    addSlider (ParamID::duty_ramp, "DUTY SWEEP", 384, 216, 130);

    addSectionHeader ("REPEAT", 384, 244);
    addSlider (ParamID::repeat_speed, "REPEAT SPEED", 384, 264, 130);

    addSectionHeader ("ARPEGGIO", 628, 68);
    addSlider (ParamID::arp_mod,   "CHANGE AMOUNT", 628, 88, 130);
    addSlider (ParamID::arp_speed, "CHANGE SPEED",  628, 108, 130);

    addSectionHeader ("PHASER", 628, 136);
    addSlider (ParamID::pha_offset, "OFFSET", 628, 156, 130);
    addSlider (ParamID::pha_ramp,   "SWEEP",  628, 176, 130);

    addSectionHeader ("FILTERS", 628, 204);
    addSlider (ParamID::lpf_freq,      "LP CUTOFF",    628, 224, 130);
    addSlider (ParamID::lpf_ramp,      "LP SWEEP",     628, 244, 130);
    addSlider (ParamID::lpf_resonance, "LP RESONANCE", 628, 264, 130);
    addSlider (ParamID::hpf_freq,      "HP CUTOFF",    628, 284, 130);
    addSlider (ParamID::hpf_ramp,      "HP SWEEP",     628, 304, 130);

    addSectionHeader ("VOLUME", 384, 420);
    addSlider (ParamID::master_vol, "OUTPUT LEVEL", 384, 440, 240);

    // ---- waveform display ----
    waveformScope = std::make_unique<WaveformScopeComponent> (audioProcessor);
    waveformScope->setBounds (12, 486, 856, 108);
    addAndMakeVisible (waveformScope.get());

    // ---- on-screen MIDI keyboard ----
    midiKeyboard = std::make_unique<SfxrMidiKeyboardComponent> (
        audioProcessor.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard);
    midiKeyboard->setAvailableRange (21, 108); // A0..C8 (full 88 keys)
    midiKeyboard->setBounds (12, 604, 856, 86);
    addAndMakeVisible (midiKeyboard.get());

    updateWaveButtons();
}

void SfxrVstiAudioProcessorEditor::randomize()
{
    SfxrParams p = audioProcessor.readParams();
    ::randomize (p, rng);
    audioProcessor.applyParams (p);
    audioProcessor.playPreview();
}

void SfxrVstiAudioProcessorEditor::mutate()
{
    SfxrParams p = audioProcessor.readParams();
    ::mutate (p, rng);
    audioProcessor.applyParams (p);
    audioProcessor.playPreview();
}

void SfxrVstiAudioProcessorEditor::generateCategory (PresetCategory c)
{
    SfxrParams p = audioProcessor.readParams();
    generatePreset (p, c, rng);
    audioProcessor.applyParams (p);
    audioProcessor.playPreview();
}

void SfxrVstiAudioProcessorEditor::loadSfs()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Load sfxr sound",
        juce::File::getSpecialLocation (juce::File::userHomeDirectory),
        "*.sfs");

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, chooser] (const juce::FileChooser& fc)
    {
        const auto result = fc.getResult();
        if (result == juce::File())
            return;

        SfxrParams p;
        if (SfxrPresetFile::load (result, p))
            audioProcessor.applyParams (p);
        else
            showStyledAlert (*lookAndFeel, juce::MessageBoxIconType::WarningIcon,
                            "Load Failed",
                            "Could not load \"" + result.getFileName()
                                + "\". The file may be corrupted or not a valid .sfs sound.", this);
    });
}

void SfxrVstiAudioProcessorEditor::saveSfs()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Save sfxr sound",
        juce::File::getSpecialLocation (juce::File::userHomeDirectory),
        "*.sfs");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, chooser] (const juce::FileChooser& fc)
    {
        const auto result = fc.getResult();
        if (result == juce::File())
            return;

        const auto target = result.withFileExtension (".sfs");
        if (! SfxrPresetFile::save (target, audioProcessor.readParams()))
            showStyledAlert (*lookAndFeel, juce::MessageBoxIconType::WarningIcon,
                            "Save Failed",
                            "Could not save \"" + target.getFileName() + "\".", this);
    });
}

void SfxrVstiAudioProcessorEditor::exportAudio()
{
    juce::Component::SafePointer<SfxrVstiAudioProcessorEditor> editor (this);
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Export Audio";
    options.dialogBackgroundColour = kBg;
    options.content.setOwned (new ExportOptionsComponent (
        [editor] (int formatId, int sampleRateId, int encodingId, const juce::File& target)
        {
            if (editor != nullptr)
                editor->exportAudio (formatId, sampleRateId, encodingId, target);
        }));
    options.componentToCentreAround = this;
    options.useNativeTitleBar = true;
    options.resizable = false;

    if (auto* dialog = options.launchAsync())
        dialog->setLookAndFeel (lookAndFeel.get());
}

void SfxrVstiAudioProcessorEditor::exportAudio (int formatId, int sampleRateId, int encodingId,
                                                 const juce::File& target)
{
    SfxrAudioExporter::Options options;
    options.format = formatId == 1
                   ? SfxrAudioExporter::Format::wav : SfxrAudioExporter::Format::ogg;
    options.sampleRate = std::array<int, 5> { 44100, 48000, 88200, 96000, 192000 }
                         [(size_t) (sampleRateId - 1)];
    options.wavBitDepth = encodingId;
    options.oggQualityIndex = encodingId - 1;
    const auto params = audioProcessor.readParams();
    const bool mono = audioProcessor.readBoolParam (ParamID::mono);
    const bool oneShot = audioProcessor.readBoolParam (ParamID::one_shot);

    const auto audio = SfxrAudioExporter::renderPreview (params, mono, oneShot, options.sampleRate);
    juce::String errorMessage;

    if (! SfxrAudioExporter::writeFile (target, audio, options, errorMessage))
        showStyledAlert (*lookAndFeel, juce::MessageBoxIconType::WarningIcon,
                        "Export Failed",
                        "Could not export \"" + target.getFileName()
                            + "\": " + errorMessage, this);
}
