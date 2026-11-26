#pragma once
#include "PluginProcessor.h"
#include "WarpDSP.h"

namespace ui
{
    const juce::Colour bg     { 0xff0e0d14 };
    const juce::Colour panel  { 0xff17161f };
    const juce::Colour line   { 0xff2a2937 };
    const juce::Colour accent { 0xff7a4dff };
    const juce::Colour pink   { 0xffff4d8d };
    const juce::Colour mute   { 0xff8d8ca0 };
}

struct WarpLNF : public juce::LookAndFeel_V4
{
    WarpLNF()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.85f));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, ui::mute);
        setColour (juce::ComboBox::backgroundColourId, ui::bg);
        setColour (juce::ComboBox::outlineColourId, ui::line);
        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::PopupMenu::backgroundColourId, ui::panel);
        setColour (juce::ToggleButton::textColourId, juce::Colours::white);
        setColour (juce::ToggleButton::tickColourId, ui::accent);
        setColour (juce::TextButton::buttonColourId, ui::panel);
        setColour (juce::TextButton::buttonOnColourId, ui::accent);
        setColour (juce::TextButton::textColourOffId, ui::mute);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.f);
        const float r = juce::jmin (b.getWidth(), b.getHeight()) / 2.f;
        const auto c = b.getCentre();
        const float ang = start + pos * (end - start);

        juce::Path bgArc;  bgArc.addCentredArc (c.x, c.y, r, r, 0.f, start, end, true);
        g.setColour (ui::line);
        g.strokePath (bgArc, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path vArc;   vArc.addCentredArc (c.x, c.y, r, r, 0.f, start, ang, true);
        g.setGradientFill (juce::ColourGradient (ui::accent, b.getX(), b.getBottom(), ui::pink, b.getRight(), b.getY(), false));
        g.strokePath (vArc, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (juce::Colours::white);
        g.fillEllipse (c.x + (r - 11.f) * std::sin (ang) - 3.f, c.y - (r - 11.f) * std::cos (ang) - 3.f, 6.f, 6.f);
    }
};

struct Knob : public juce::Component
{
    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& name)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (slider);
        addAndMakeVisible (label);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    }
    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromTop (16));
        slider.setBounds (b);
    }
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

struct BandPanel : public juce::Component
{
    BandPanel (juce::AudioProcessorValueTreeState& s, int band);
    void resized() override;
    juce::ToggleButton on { "Active" };
    juce::ComboBox shape;
    std::unique_ptr<Knob> drive, bias, mix, gain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAtt;
};

struct CurveView : public juce::Component, private juce::Timer
{
    CurveView (WarpAudioProcessor& p) : proc (p) { startTimerHz (30); }
    void setBand (int b) { band = b; repaint(); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    WarpAudioProcessor& proc;
    int band = 0;
};

struct ScopeView : public juce::Component, private juce::Timer
{
    ScopeView (WarpAudioProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    WarpAudioProcessor& proc;
};

class WarpAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit WarpAudioProcessorEditor (WarpAudioProcessor&);
    ~WarpAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void selectBand (int b);

    WarpAudioProcessor& proc;
    WarpLNF lnf;
    CurveView curve;
    ScopeView scope;
    juce::TextButton bandBtn[3];
    std::unique_ptr<BandPanel> panels[3];
    std::vector<std::unique_ptr<Knob>> globals;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarpAudioProcessorEditor)
};
