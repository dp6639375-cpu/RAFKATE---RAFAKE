#pragma once
#include "PluginProcessor.h"
#include "WarpDSP.h"

namespace ui
{
    // Palette (defaults = Graphite). Display screens stay dark in both themes, like real hardware.
    inline juce::Colour chassisTop { 0xff5a5f66 }, chassisBottom { 0xff2b2e32 },
                        panelTop { 0xff52575d }, panelBottom { 0xff3e4247 },
                        panel { 0xff45494f }, bg { 0xff2b2e32 }, line { 0xff1f2124 },
                        mute { 0xffaab0b7 }, text { 0xffe9ecef };
    inline juce::Colour accent { 0xff3d9be0 }, accent2 { 0xff7fe3ff };          // aero blue / cyan
    inline juce::Colour screen { 0xff0e1419 }, screenLine { 0xff1f303b }, screenText { 0xffa9c8da };
    inline bool isDark = true;

    inline void setDark (bool dark)
    {
        isDark = dark;
        chassisTop    = juce::Colour (dark ? 0xff5a5f66 : 0xffd5d8dd);
        chassisBottom = juce::Colour (dark ? 0xff2b2e32 : 0xff9da2a9);
        panelTop      = juce::Colour (dark ? 0xff52575d : 0xffdfe2e6);
        panelBottom   = juce::Colour (dark ? 0xff3e4247 : 0xffc2c6cc);
        panel         = juce::Colour (dark ? 0xff45494f : 0xffc9cdd2);
        bg            = juce::Colour (dark ? 0xff2b2e32 : 0xffc9cdd2);
        line          = juce::Colour (dark ? 0xff1f2124 : 0xff7d838b);
        mute          = juce::Colour (dark ? 0xffaab0b7 : 0xff4d535a);
        text          = juce::Colour (dark ? 0xffe9ecef : 0xff1d2024);
    }

    // Recessed glass display (bezel + dark screen + gloss + darkened edges). Returns the inner drawing area.
    inline juce::Rectangle<float> drawScreen (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff101214), r.getX(), r.getY(),
                                                 juce::Colour (0xffd9dde2).withAlpha (isDark ? 0.35f : 0.95f), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 3.f);
        auto in = r.reduced (3.f);
        g.setColour (screen);
        g.fillRoundedRectangle (in, 2.f);

        juce::ColourGradient vig (juce::Colours::transparentBlack, in.getCentreX(), in.getCentreY(),
                                  juce::Colours::black.withAlpha (0.55f), in.getX(), in.getY(), true);
        vig.addColour (0.55, juce::Colours::transparentBlack);
        g.setGradientFill (vig);
        g.fillRoundedRectangle (in, 2.f);

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.11f), in.getX(), in.getY(),
                                                 juce::Colours::white.withAlpha (0.0f), in.getX(), in.getCentreY(), false));
        g.fillRoundedRectangle (in.withHeight (in.getHeight() * 0.5f), 2.f);
        return in.reduced (2.f);
    }

    // Raised faceplate with bevel.
    inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setGradientFill (juce::ColourGradient (panelTop, r.getX(), r.getY(), panelBottom, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 3.f);
        g.setColour (juce::Colours::white.withAlpha (isDark ? 0.22f : 0.85f));
        g.drawLine (r.getX() + 2.f, r.getY() + 1.f, r.getRight() - 2.f, r.getY() + 1.f, 1.2f);
        g.drawLine (r.getX() + 1.f, r.getY() + 2.f, r.getX() + 1.f, r.getBottom() - 2.f, 1.f);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawLine (r.getX() + 2.f, r.getBottom() - 1.f, r.getRight() - 2.f, r.getBottom() - 1.f, 1.2f);
        g.drawLine (r.getRight() - 1.f, r.getY() + 2.f, r.getRight() - 1.f, r.getBottom() - 2.f, 1.f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawRoundedRectangle (r, 3.f, 1.f);
    }

    inline void drawScrew (juce::Graphics& g, float cx, float cy, float angle)
    {
        const float r = 4.f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f4f6), cx - 2.f, cy - 2.f, juce::Colour (0xff6f757c), cx + 3.f, cy + 3.f, false));
        g.fillEllipse (cx - r, cy - r, r * 2.f, r * 2.f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (cx - r, cy - r, r * 2.f, r * 2.f, 0.8f);
        g.setColour (juce::Colour (0xff2b2e32));
        g.drawLine (cx - 3.f * std::cos (angle), cy - 3.f * std::sin (angle), cx + 3.f * std::cos (angle), cy + 3.f * std::sin (angle), 1.3f);
    }
}

using APVTS = juce::AudioProcessorValueTreeState;

struct WarpLNF : public juce::LookAndFeel_V4
{
    WarpLNF() { apply(); }

    void apply()
    {
        using namespace juce;
        setColour (Slider::textBoxTextColourId, ui::text.withAlpha (0.9f));
        setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
        setColour (Label::textColourId, ui::mute);
        setColour (ComboBox::textColourId, ui::screenText);
        setColour (ComboBox::backgroundColourId, ui::screen);
        setColour (ComboBox::outlineColourId, Colours::transparentBlack);
        setColour (ComboBox::arrowColourId, ui::accent2);
        setColour (PopupMenu::backgroundColourId, Colour (0xff1d2227));
        setColour (PopupMenu::textColourId, Colour (0xffdfe6ec));
        setColour (PopupMenu::highlightedBackgroundColourId, ui::accent);
        setColour (PopupMenu::highlightedTextColourId, Colours::white);
        setColour (ToggleButton::textColourId, ui::text);
        setColour (TextButton::textColourOffId, ui::isDark ? Colour (0xffd6dbe0) : Colour (0xff2b2f34));
        setColour (TextButton::textColourOnId, Colours::white);
    }

    // Knob with drop shadow, metallic cap, gloss, tick scale and lit LED arc
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override
    {
        using namespace juce;
        auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
        const float R = jmin (b.getWidth(), b.getHeight()) / 2.f;
        const auto c = b.getCentre();
        const float kr = R * 0.64f;
        const float ang = start + pos * (end - start);
        auto dir = [] (float a) { return Point<float> (std::sin (a), -std::cos (a)); };

        for (int i = 0; i < 11; ++i)
        {
            const float a = start + (end - start) * (float) i / 10.f;
            const auto d = dir (a);
            g.setColour (ui::mute.withAlpha (0.85f));
            g.drawLine (c.x + d.x * R * 0.80f, c.y + d.y * R * 0.80f, c.x + d.x * R * 0.95f, c.y + d.y * R * 0.95f, i % 5 == 0 ? 1.8f : 1.1f);
        }

        Path lit; lit.addCentredArc (c.x, c.y, R * 0.88f, R * 0.88f, 0.f, start, ang, true);
        g.setColour (ui::accent2.withAlpha (0.22f));
        g.strokePath (lit, PathStrokeType (6.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (ui::accent);
        g.strokePath (lit, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));

        Path body; body.addEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
        DropShadow (Colours::black.withAlpha (0.6f), (int) (kr * 0.55f), { 0, (int) (kr * 0.28f) }).drawForPath (g, body);

        g.setGradientFill (ColourGradient (Colour (0xfff6f7f9), c.x - kr * 0.6f, c.y - kr * 0.8f,
                                           Colour (0xff5d6369), c.x + kr * 0.6f, c.y + kr * 0.9f, false));
        g.fillPath (body);
        const float cr = kr * 0.80f;
        g.setGradientFill (ColourGradient (Colour (0xff8d939a), c.x - cr * 0.5f, c.y - cr * 0.8f,
                                           Colour (0xffe9ecef), c.x + cr * 0.5f, c.y + cr * 0.9f, false));
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f);

        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.65f), c.x, c.y - cr * 0.95f,
                                           Colours::white.withAlpha (0.0f), c.x, c.y - cr * 0.05f, false));
        g.fillEllipse (c.x - cr * 0.72f, c.y - cr * 0.94f, cr * 1.44f, cr * 0.9f);

        const auto d = dir (ang);
        g.setColour (Colours::white.withAlpha (0.6f));
        g.drawLine (c.x + d.x * cr * 0.28f + 1.f, c.y + d.y * cr * 0.28f + 1.f, c.x + d.x * cr * 0.88f + 1.f, c.y + d.y * cr * 0.88f + 1.f, 3.f);
        g.setColour (Colour (0xff1c1f23));
        g.drawLine (c.x + d.x * cr * 0.28f, c.y + d.y * cr * 0.28f, c.x + d.x * cr * 0.88f, c.y + d.y * cr * 0.88f, 3.f);

        g.setColour (Colours::black.withAlpha (0.65f));
        g.drawEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f, 1.f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        using namespace juce;
        if (style != Slider::LinearHorizontal)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, minPos, maxPos, style, s);
            return;
        }
        const float cy = (float) y + (float) h * 0.5f;
        Rectangle<float> groove ((float) x + 2.f, cy - 3.f, (float) w - 4.f, 6.f);
        g.setColour (Colour (0xff0e1114));
        g.fillRoundedRectangle (groove, 2.f);
        g.setColour (Colours::white.withAlpha (0.28f));
        g.drawLine (groove.getX(), groove.getBottom() + 0.7f, groove.getRight(), groove.getBottom() + 0.7f);

        auto fill = groove.withRight (jmax (groove.getX(), pos)).reduced (0.f, 1.f);
        g.setGradientFill (ColourGradient (ui::accent2, fill.getX(), fill.getY(), ui::accent, fill.getX(), fill.getBottom(), false));
        g.fillRoundedRectangle (fill, 1.5f);

        Rectangle<float> th (pos - 6.f, cy - 10.f, 12.f, 20.f);
        DropShadow (Colours::black.withAlpha (0.55f), 5, { 0, 2 }).drawForRectangle (g, th.toNearestInt());
        g.setGradientFill (ColourGradient (Colour (0xfff6f7f9), th.getX(), th.getY(), Colour (0xff7d838a), th.getX(), th.getBottom(), false));
        g.fillRoundedRectangle (th, 2.f);
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.55f), th.getX(), th.getY(), Colours::white.withAlpha (0.0f), th.getX(), th.getCentreY(), false));
        g.fillRoundedRectangle (th.reduced (1.f).withHeight (th.getHeight() * 0.5f), 2.f);
        g.setColour (Colour (0xff23262a));
        g.drawLine (th.getCentreX(), th.getY() + 4.f, th.getCentreX(), th.getBottom() - 4.f, 1.4f);
        g.setColour (Colours::black.withAlpha (0.7f));
        g.drawRoundedRectangle (th, 2.f, 1.f);
    }

    // Rectangular glossy "aero" push buttons / tabs
    void drawButtonBackground (juce::Graphics& g, juce::Button& btn, const juce::Colour&, bool hover, bool down) override
    {
        using namespace juce;
        auto r = btn.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = btn.getToggleState();
        Colour top = on ? Colour (0xff9bd4f5) : (ui::isDark ? Colour (0xff767c83) : Colour (0xfff3f5f7));
        Colour bot = on ? Colour (0xff2c78b8) : (ui::isDark ? Colour (0xff3a3e43) : Colour (0xffa4aab1));
        if (hover) { top = top.brighter (0.12f); bot = bot.brighter (0.08f); }
        if (down)  { std::swap (top, bot); }
        g.setGradientFill (ColourGradient (top, r.getX(), r.getY(), bot, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 2.f);
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.48f), r.getX(), r.getY(), Colours::white.withAlpha (0.06f), r.getX(), r.getCentreY(), false));
        g.fillRoundedRectangle (r.reduced (1.f).withHeight (r.getHeight() * 0.5f), 2.f);
        g.setColour (Colour (0xff15171a).withAlpha (0.85f));
        g.drawRoundedRectangle (r, 2.f, 1.f);
    }

    // Toggle = LED + label
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool) override
    {
        using namespace juce;
        auto r = b.getLocalBounds().toFloat();
        const float d = 12.f;
        Rectangle<float> led (r.getX() + 5.f, r.getCentreY() - d / 2.f, d, d);
        const bool on = b.getToggleState();
        const Colour c = on ? Colour (0xff4fd1ff) : Colour (0xff2a3238);
        if (on) { g.setColour (c.withAlpha (0.25f)); g.fillEllipse (led.expanded (5.f)); }
        g.setColour (Colour (0xff111316));
        g.fillEllipse (led.expanded (2.f));
        g.setGradientFill (ColourGradient (c.brighter (0.7f), led.getCentreX() - 2.f, led.getY() + 2.f, c.darker (0.5f), led.getRight(), led.getBottom(), true));
        g.fillEllipse (led);
        g.setColour (Colours::white.withAlpha (on ? 0.7f : 0.25f));
        g.fillEllipse (led.getX() + 2.5f, led.getY() + 1.5f, d * 0.45f, d * 0.3f);
        g.setColour (ui::text);
        g.setFont (FontOptions (13.f));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (d + 14.f), Justification::centredLeft);
    }

    // Recessed LCD style selector
    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        using namespace juce;
        Rectangle<float> r (0.f, 0.f, (float) w, (float) h);
        ui::drawScreen (g, r);
        Path tri;
        const float cx = (float) w - 14.f, cy = (float) h * 0.5f;
        tri.addTriangle (cx - 4.f, cy - 2.f, cx + 4.f, cy - 2.f, cx, cy + 3.f);
        g.setColour (ui::accent2);
        g.fillPath (tri);
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
                     playBtn { "Play" }, themeBtn { "Silver" };
    juce::TextButton pageBtn[4], bandBtn[3];
    std::unique_ptr<BandPanel> panels[3];
    FilterPage filt;
    FeedbackPage fb;
    ModPage mod;
    std::vector<std::unique_ptr<Knob>> globals;
    std::unique_ptr<Knob> bigMacro[2];
    int page = 0, band = 0;
    bool playMode = false;
    juce::Image chassis;
    bool chassisDark = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarpAudioProcessorEditor)
};
