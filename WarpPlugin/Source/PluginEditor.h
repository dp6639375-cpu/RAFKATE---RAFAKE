#pragma once
#include "PluginProcessor.h"
#include "WarpDSP.h"

namespace ui
{
    inline juce::Colour bg     { 0xff0e0d14 };
    inline juce::Colour panel  { 0xff17161f };
    inline juce::Colour line   { 0xff2a2937 };
    inline juce::Colour mute   { 0xff8d8ca0 };
    inline juce::Colour text   { 0xffecebf5 };
    inline juce::Colour accent { 0xff7a4dff };
    inline juce::Colour pink   { 0xffff4d8d };
    inline bool isDark = true;

    inline void setDark (bool dark)
    {
        isDark = dark;
        bg    = juce::Colour (dark ? 0xff0e0d14 : 0xfff2f1f6);
        panel = juce::Colour (dark ? 0xff17161f : 0xffffffff);
        line  = juce::Colour (dark ? 0xff2a2937 : 0xffdcdbe6);
        mute  = juce::Colour (dark ? 0xff8d8ca0 : 0xff6b6a78);
        text  = juce::Colour (dark ? 0xffecebf5 : 0xff1c1b22);
    }
}

using APVTS = juce::AudioProcessorValueTreeState;

struct WarpLNF : public juce::LookAndFeel_V4
{
    WarpLNF() { apply(); }

    void apply()
    {
        setColour (juce::Slider::textBoxTextColourId, ui::text.withAlpha (0.85f));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::trackColourId, ui::accent);
        setColour (juce::Slider::backgroundColourId, ui::line);
        setColour (juce::Slider::thumbColourId, ui::text);
        setColour (juce::Label::textColourId, ui::mute);
        setColour (juce::ComboBox::backgroundColourId, ui::bg);
        setColour (juce::ComboBox::outlineColourId, ui::line);
        setColour (juce::ComboBox::textColourId, ui::text);
        setColour (juce::ComboBox::arrowColourId, ui::mute);
        setColour (juce::PopupMenu::backgroundColourId, ui::panel);
        setColour (juce::PopupMenu::textColourId, ui::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::accent);
        setColour (juce::ToggleButton::textColourId, ui::text);
        setColour (juce::ToggleButton::tickColourId, ui::accent);
        setColour (juce::ToggleButton::tickDisabledColourId, ui::mute);
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
        const float th = juce::jlimit (4.f, 10.f, r * 0.12f);

        juce::Path bgArc;  bgArc.addCentredArc (c.x, c.y, r, r, 0.f, start, end, true);
        g.setColour (ui::line);
        g.strokePath (bgArc, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path vArc;   vArc.addCentredArc (c.x, c.y, r, r, 0.f, start, ang, true);
        g.setGradientFill (juce::ColourGradient (ui::accent, b.getX(), b.getBottom(), ui::pink, b.getRight(), b.getY(), false));
        g.strokePath (vArc, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (ui::text);
        const float dr = juce::jlimit (3.f, 7.f, r * 0.08f);
        g.fillEllipse (c.x + (r - th * 2.4f) * std::sin (ang) - dr, c.y - (r - th * 2.4f) * std::cos (ang) - dr, dr * 2.f, dr * 2.f);
    }
};

struct Knob : public juce::Component
{
    Knob (APVTS& s, const juce::String& id, const juce::String& name)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (slider);
        addAndMakeVisible (label);
        att = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
    }
    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromTop (16));
        slider.setBounds (b);
    }
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

struct HSlider : public juce::Component
{
    HSlider (APVTS& s, const juce::String& id)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 18);
        addAndMakeVisible (slider);
        att = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
    }
    void resized() override { slider.setBounds (getLocalBounds()); }
    juce::Slider slider;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

struct BandPanel : public juce::Component
{
    BandPanel (APVTS& s, int band);
    void resized() override;
    juce::ToggleButton on { "Active" };
    juce::ComboBox shapeP, shapeN;
    juce::Label lblP, lblN;
    std::unique_ptr<Knob> drive, bias, mix, gain;
    std::unique_ptr<APVTS::ButtonAttachment> onAtt;
    std::unique_ptr<APVTS::ComboBoxAttachment> pAtt, nAtt;
};

struct FilterPage : public juce::Component
{
    explicit FilterPage (APVTS& s);
    void resized() override;
    juce::ToggleButton preOn { "Pre-filter" }, postOn { "Post-filter" };
    juce::ComboBox preType, postType;
    std::unique_ptr<Knob> pre[3], post[3];
    std::unique_ptr<APVTS::ButtonAttachment> preAtt, postAtt;
    std::unique_ptr<APVTS::ComboBoxAttachment> preTAtt, postTAtt;
};

struct FeedbackPage : public juce::Component
{
    explicit FeedbackPage (APVTS& s);
    void resized() override;
    juce::ToggleButton on { "Feedback" }, snap { "Pitch snap" }, midi { "MIDI track" };
    std::unique_ptr<Knob> k[4];
    std::unique_ptr<APVTS::ButtonAttachment> onAtt, snapAtt, midiAtt;
};

struct CurveEditor : public juce::Component, private juce::Timer
{
    explicit CurveEditor (WarpAudioProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    void timerCallback() override { repaint(); }
    void setFrom (const juce::MouseEvent&);
    WarpAudioProcessor& proc;
    juce::RangedAudioParameter* pts[8];
    int active = -1;
};

struct ModPage : public juce::Component
{
    explicit ModPage (WarpAudioProcessor& p);
    void resized() override;
    CurveEditor editor;
    juce::OwnedArray<juce::Label> labels;
    std::unique_ptr<HSlider> rate[3];       // LFO rate, curve rate, follow release
    std::unique_ptr<HSlider> amt[5][3];
};

struct CurveView : public juce::Component, private juce::Timer
{
    explicit CurveView (WarpAudioProcessor& p) : proc (p) { startTimerHz (30); }
    void setBand (int b) { band = b; repaint(); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    WarpAudioProcessor& proc;
    int band = 0;
};

// Oscilloscope / multiband spectrum. Click to switch between the two.
struct ScopeView : public juce::Component, private juce::Timer
{
    explicit ScopeView (WarpAudioProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { mode = 1 - mode; repaint(); }
private:
    void timerCallback() override;
    void computeSpectrum();
    WarpAudioProcessor& proc;
    int mode = 1;
    juce::dsp::FFT fft { 11 };
    juce::dsp::WindowingFunction<float> window { 2048, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, 4096> data {};
    std::array<float, 1024> smooth {};
};

class WarpAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit WarpAudioProcessorEditor (WarpAudioProcessor&);
    ~WarpAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void updateVisibility();
    void setPage (int p);
    void selectBand (int b);
    void buildPresetMenu();
    void stepPreset (int delta);
    void applyTheme (bool dark);

    WarpAudioProcessor& proc;
    WarpLNF lnf;
    CurveView curve;
    ScopeView scope;
    juce::ComboBox presetBox;
    juce::TextButton randomBtn { "Randomize" }, prevBtn { "<" }, nextBtn { ">" },
                     playBtn { "Play" }, themeBtn { "Light" };
    juce::TextButton pageBtn[4], bandBtn[3];
    std::unique_ptr<BandPanel> panels[3];
    FilterPage filt;
    FeedbackPage fb;
    ModPage mod;
    std::vector<std::unique_ptr<Knob>> globals;
    std::unique_ptr<Knob> bigMacro[2];
    int page = 0, band = 0;
    bool playMode = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarpAudioProcessorEditor)
};
