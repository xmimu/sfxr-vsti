#include "ExportOptionsComponent.h"
#include "SfxrDialogs.h"
#include "SfxrTheme.h"

namespace
{
    juce::PropertiesFile::Options getExportSettingsOptions()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "SfxrVsti";
        options.filenameSuffix = "settings";
        options.folderName = "SfxrVsti";
        options.osxLibrarySubFolder = "Application Support";
        return options;
    }
}

ExportOptionsComponent::ExportOptionsComponent (ExportCallback callback)
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

void ExportOptionsComponent::paint (juce::Graphics& g)
{
    g.fillAll (SfxrTheme::kBg);
    g.setColour (SfxrTheme::kTextDark);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("FORMAT", 16, 12, 160, 16, juce::Justification::centredLeft);
    g.drawText ("SAMPLE RATE", 16, 58, 160, 16, juce::Justification::centredLeft);
    g.drawText ("BIT DEPTH / BITRATE", 16, 104, 180, 16, juce::Justification::centredLeft);
    g.drawText ("EXPORT FOLDER", 16, 150, 180, 16, juce::Justification::centredLeft);
    g.drawText ("FILE NAME", 16, 200, 160, 16, juce::Justification::centredLeft);
    g.setFont (11.0f);
    g.drawText (outputDirectory.getFullPathName(), 16, 168, 226, 20, juce::Justification::centredLeft, true);
}

void ExportOptionsComponent::resized()
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

void ExportOptionsComponent::updateEncodingChoices()
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

void ExportOptionsComponent::chooseOutputDirectory()
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

void ExportOptionsComponent::exportFile()
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
    SfxrDialogs::showConfirm (getLookAndFeel(), juce::MessageBoxIconType::WarningIcon,
                              "File Already Exists",
                              "\"" + target.getFileName() + "\" already exists. Replace it?",
                              this, [component, target]
    {
        if (component != nullptr)
            component->performExport (target);
    });
}

void ExportOptionsComponent::performExport (const juce::File& target)
{
    juce::PropertiesFile settings (getExportSettingsOptions());
    settings.setValue ("exportFormat", format.getSelectedId());
    settings.setValue ("exportSampleRate", sampleRate.getSelectedId());
    settings.setValue ("exportEncoding", encoding.getSelectedId());
    settings.setValue ("exportDirectory", outputDirectory.getFullPathName());

    onExport (format.getSelectedId(), sampleRate.getSelectedId(), encoding.getSelectedId(), target);
    closeDialog();
}

void ExportOptionsComponent::closeDialog()
{
    if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
        dialog->exitModalState (0);
}
