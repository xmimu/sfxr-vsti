#pragma once

#include <JuceHeader.h>

// Modal options panel for the audio export dialog. Collects format / sample
// rate / encoding / folder / filename and remembers the last choices between
// dialog opens in a PropertiesFile.
class ExportOptionsComponent : public juce::Component
{
public:
    using ExportCallback = std::function<void (int, int, int, const juce::File&)>;

    explicit ExportOptionsComponent (ExportCallback callback);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void updateEncodingChoices();
    void chooseOutputDirectory();
    void exportFile();
    void performExport (const juce::File& target);
    void closeDialog();

    ExportCallback onExport;
    juce::File outputDirectory;
    juce::ComboBox format, sampleRate, encoding;
    juce::TextEditor fileName;
    juce::TextButton chooseFolder, cancel, exportButton;
};
