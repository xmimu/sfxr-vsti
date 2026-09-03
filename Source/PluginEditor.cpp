#include "PluginEditor.h"
#include "Gui/EditorActions.h"
#include "Gui/SfxrTheme.h"
#include "Gui/SfxrLookAndFeel.h"
#include "Gui/WaveformScope.h"
#include "Gui/SfxrMidiKeyboard.h"
#include "Gui/ExportOptionsComponent.h"
#include "Gui/SfxrDialogs.h"
#include "SfxrEngine/SfxrPresets.h"
#include "SfxrEngine/SfxrAudioExporter.h"

namespace
{
    // Fixed editor canvas and column anchors. Coordinates inside the per-area
    // build functions are intentionally copied verbatim from the pre-refactor
    // layout so the fixed pixel UI stays identical.
    constexpr int kEditorWidth  = 880;
    constexpr int kEditorHeight = 700;

    // sfxr palette lives in SfxrTheme.h, shared with the LookAndFeel and the
    // export dialog.
}

SfxrVstiAudioProcessorEditor::SfxrVstiAudioProcessorEditor (SfxrVstiAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      apvts (p.getAPVTS())
{
    lookAndFeel = std::make_unique<SfxrLookAndFeel>();
    setLookAndFeel (lookAndFeel.get());

    actions = std::make_unique<SfxrEditorActions> (p);

    apvts.addParameterListener (juce::String (ParamID::wave_type), this);

    buildInterface();

    setResizable (false, false);
    setSize (kEditorWidth, kEditorHeight);
}

SfxrVstiAudioProcessorEditor::~SfxrVstiAudioProcessorEditor()
{
    cancelPendingUpdate();
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
        window->setContentComponentSize (kEditorWidth, kEditorHeight);
        configuredStandaloneTitleBar = true;
    }
}

void SfxrVstiAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (SfxrTheme::kBg);

    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.setColour (SfxrTheme::kTextDark);
    g.drawText ("sfxr", 12, 6, 60, 24, juce::Justification::centredLeft);

    g.setFont (12.0f);
    g.drawText ("VSTi", 12, 26, 60, 16, juce::Justification::centredLeft);
    g.drawText (juce::String ("v") + JucePlugin_VersionString,
                62, 26, 58, 16, juce::Justification::centredRight);

    g.setColour (SfxrTheme::kDivider);
    g.drawLine (128.0f, 36.0f, 128.0f, (float) kEditorHeight);
    g.drawLine (12.0f, 234.0f, 122.0f, 234.0f);
    g.drawLine (12.0f, 318.0f, 122.0f, 318.0f);

    // divider above the waveform display
    g.setColour (SfxrTheme::kDivider);
    g.drawLine (8.0f, 480.0f, (float) (kEditorWidth - 8), 480.0f);

    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("WAVEFORM", 16, 466, 120, 14, juce::Justification::centredLeft);
    g.drawText ("MIDI KEYBOARD", 16, 590, 160, 14, juce::Justification::centredLeft);
}

void SfxrVstiAudioProcessorEditor::parameterChanged (const juce::String& id, float)
{
    // parameterChanged can fire on the audio thread (host automation writing the
    // wave_type parameter), so it must not touch any widget directly. Queue the
    // button refresh to run on the message thread instead.
    if (id == ParamID::wave_type)
        triggerAsyncUpdate();
}

void SfxrVstiAudioProcessorEditor::handleAsyncUpdate()
{
    updateWaveButtons();
}

void SfxrVstiAudioProcessorEditor::addSectionHeader (const juce::String& text, int x, int y)
{
    auto* l = own (new juce::Label());
    l->setText (text, juce::dontSendNotification);
    l->setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    l->setColour (juce::Label::textColourId, SfxrTheme::kTextDark);
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
    lbl->setColour (juce::Label::textColourId, SfxrTheme::kTextDark);
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
    b->setColour (juce::ToggleButton::tickColourId, SfxrTheme::kBarFill);
    b->setColour (juce::ToggleButton::tickDisabledColourId, SfxrTheme::kButtonBg);
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
    buildTopBar();
    buildGeneratorColumn();
    buildSettingsColumns();
    buildWaveformArea();
    buildKeyboardArea();

    updateWaveButtons();
}

void SfxrVstiAudioProcessorEditor::buildTopBar()
{
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
}

void SfxrVstiAudioProcessorEditor::buildGeneratorColumn()
{
    addSectionHeader ("GENERATOR", 12, 40);

    int gy = 62;
    for (int i = 0; i < (int) PresetCategory::Count; i++)
    {
        const auto cat = (PresetCategory) i;
        auto* b = own (new juce::TextButton (presetCategoryName (cat)));
        b->setBounds (12, gy, 110, 22);
        b->onClick = [this, cat] { actions->generate (cat); };
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

    addActionButton ("MUTATE",      [this] { actions->mutate(); });
    addActionButton ("RANDOMIZE",   [this] { actions->randomize(); });
    addActionButton ("PLAY SOUND",  [this] { audioProcessor.playPreview(); });

    gy += 12;

    addActionButton ("LOAD CONFIG", [this] { loadSfs(); });
    addActionButton ("SAVE CONFIG", [this] { saveSfs(); });

    if (juce::JUCEApplicationBase::isStandaloneApp())
        addActionButton ("EXPORT AUDIO", [this] { exportAudio(); });
}

void SfxrVstiAudioProcessorEditor::buildSettingsColumns()
{
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
    addSlider (ParamID::duty,      "DUTY",       384, 196, 130);
    addSlider (ParamID::duty_ramp, "DUTY SWEEP",  384, 216, 130);

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
}

void SfxrVstiAudioProcessorEditor::buildWaveformArea()
{
    waveformScope = std::make_unique<WaveformScope> (audioProcessor);
    waveformScope->setBounds (12, 486, kEditorWidth - 24, 108);
    addAndMakeVisible (waveformScope.get());
}

void SfxrVstiAudioProcessorEditor::buildKeyboardArea()
{
    midiKeyboard = std::make_unique<SfxrMidiKeyboardComponent> (
        audioProcessor.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard);
    midiKeyboard->setAvailableRange (21, 108); // A0..C8 (full 88 keys)
    midiKeyboard->setBounds (12, 604, kEditorWidth - 24, 86);
    addAndMakeVisible (midiKeyboard.get());
}

void SfxrVstiAudioProcessorEditor::loadSfs()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Load sfxr sound",
        juce::File::getSpecialLocation (juce::File::userHomeDirectory),
        "*.sfs");

    juce::Component::SafePointer<SfxrVstiAudioProcessorEditor> editor (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [editor, chooser] (const juce::FileChooser& fc)
    {
        const auto result = fc.getResult();
        if (editor == nullptr || result == juce::File())
            return;

        if (! editor->actions->loadFromFile (result))
            SfxrDialogs::showAlert (*editor->lookAndFeel, juce::MessageBoxIconType::WarningIcon,
                                    "Load Failed",
                                    "Could not load \"" + result.getFileName()
                                        + "\". The file may be corrupted or not a valid .sfs sound.",
                                    editor);
    });
}

void SfxrVstiAudioProcessorEditor::saveSfs()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Save sfxr sound",
        juce::File::getSpecialLocation (juce::File::userHomeDirectory),
        "*.sfs");

    juce::Component::SafePointer<SfxrVstiAudioProcessorEditor> editor (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                          [editor, chooser] (const juce::FileChooser& fc)
    {
        const auto result = fc.getResult();
        if (editor == nullptr || result == juce::File())
            return;

        const auto target = result.withFileExtension (".sfs");
        if (! editor->actions->saveToFile (target))
            SfxrDialogs::showAlert (*editor->lookAndFeel, juce::MessageBoxIconType::WarningIcon,
                                    "Save Failed",
                                    "Could not save \"" + target.getFileName() + "\".", editor);
    });
}

void SfxrVstiAudioProcessorEditor::exportAudio()
{
    juce::Component::SafePointer<SfxrVstiAudioProcessorEditor> editor (this);
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Export Audio";
    options.dialogBackgroundColour = SfxrTheme::kBg;
    options.content.setOwned (new ExportOptionsComponent (
        [editor] (int formatId, int sampleRateId, int encodingId, const juce::File& target)
        {
            if (editor != nullptr)
                editor->performExport (formatId, sampleRateId, encodingId, target);
        }));
    options.componentToCentreAround = this;
    options.useNativeTitleBar = true;
    options.resizable = false;

    if (auto* dialog = options.launchAsync())
        dialog->setLookAndFeel (lookAndFeel.get());
}

void SfxrVstiAudioProcessorEditor::performExport (int formatId, int sampleRateId, int encodingId,
                                                   const juce::File& target)
{
    SfxrAudioExporter::Options options;
    options.format = formatId == 1
                   ? SfxrAudioExporter::Format::wav : SfxrAudioExporter::Format::ogg;
    options.sampleRate = std::array<int, 5> { 44100, 48000, 88200, 96000, 192000 }
                         [(size_t) (sampleRateId - 1)];
    options.wavBitDepth = encodingId;
    options.oggQualityIndex = encodingId - 1;

    juce::String errorMessage;
    if (! actions->exportSound (target, options, errorMessage))
        SfxrDialogs::showAlert (*lookAndFeel, juce::MessageBoxIconType::WarningIcon,
                                "Export Failed",
                                "Could not export \"" + target.getFileName()
                                    + "\": " + errorMessage, this);
}
