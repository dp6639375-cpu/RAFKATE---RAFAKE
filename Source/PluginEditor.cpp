#include "PluginEditor.h"

using namespace juce;

// ---------------- BandPanel ----------------
BandPanel::BandPanel (AudioProcessorValueTreeState& s, int band)
{
    const String p = "b" + String (band + 1) + "_";
    for (int i = 0; i < warp::numShapes; ++i) shape.addItem (warp::shapeNames[i], i + 1);

    addAndMakeVisible (on);
    addAndMakeVisible (shape);
    onAtt    = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>   (s, p + "on", on);
    shapeAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (s, p + "shape", shape);

    drive = std::make_unique<Knob> (s, p + "drive", "Drive");
    bias  = std::make_unique<Knob> (s, p + "bias",  "Bias");
    mix   = std::make_unique<Knob> (s, p + "mix",   "Mix");
    gain  = std::make_unique<Knob> (s, p + "gain",  "Gain");
    for (auto* k : { drive.get(), bias.get(), mix.get(), gain.get() }) addAndMakeVisible (k);
}

void BandPanel::resized()
{
    auto b = getLocalBounds().reduced (12);
    auto top = b.removeFromTop (30);
    on.setBounds (top.removeFromLeft (110));
    shape.setBounds (top.removeFromLeft (180));
    b.removeFromTop (10);
    const int w = b.getWidth() / 4;
    for (auto* k : { drive.get(), bias.get(), mix.get(), gain.get() })
        k->setBounds (b.removeFromLeft (w));
}

// ---------------- CurveView ----------------
void CurveView::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.f);
    g.setColour (ui::bg);
    g.fillRoundedRectangle (b, 12.f);

    g.setColour (ui::line.withAlpha (0.6f));
    for (int i = 0; i <= 8; ++i)
    {
        const float fx = b.getX() + b.getWidth()  * i / 8.f;
        const float fy = b.getY() + b.getHeight() * i / 8.f;
        g.drawLine (fx, b.getY(), fx, b.getBottom(), i == 4 ? 1.6f : 0.6f);
        g.drawLine (b.getX(), fy, b.getRight(), fy, i == 4 ? 1.6f : 0.6f);
    }

    const String id = "b" + String (band + 1) + "_";
    auto val = [&] (const String& k) { return proc.apvts.getRawParameterValue (id + k)->load(); };
    const float driveG = Decibels::decibelsToGain (val ("drive"));
    const float bias   = val ("bias");
    const int   type   = (int) val ("shape");
    const bool  active = val ("on") > 0.5f;

    auto toX = [&] (float x) { return b.getX() + (x + 1.f) * 0.5f * b.getWidth(); };
    auto toY = [&] (float y) { return b.getBottom() - (jlimit (-1.f, 1.f, y) + 1.f) * 0.5f * b.getHeight(); };

    Path p;
    for (int k = 0; k <= 256; ++k)
    {
        const float x = -1.f + 2.f * (float) k / 256.f;
        const float y = active ? warp::shape (x * driveG + bias, type) : x;
        if (k == 0) p.startNewSubPath (toX (x), toY (y)); else p.lineTo (toX (x), toY (y));
    }
    g.setGradientFill (ColourGradient (ui::accent, b.getX(), b.getBottom(), ui::pink, b.getRight(), b.getY(), false));
    g.strokePath (p, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));

    if (active)
    {
        const float dx = toX (proc.bandIn[band].load()), dy = toY (proc.bandOut[band].load());
        g.setColour (ui::pink.withAlpha (0.3f));  g.fillEllipse (dx - 11.f, dy - 11.f, 22.f, 22.f);
        g.setColour (Colours::white);             g.fillEllipse (dx - 4.f,  dy - 4.f,  8.f,  8.f);
    }
}

// ---------------- ScopeView ----------------
void ScopeView::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.f);
    g.setColour (ui::bg);
    g.fillRoundedRectangle (b, 12.f);
    g.setColour (ui::line);
    g.drawLine (b.getX(), b.getCentreY(), b.getRight(), b.getCentreY());

    Path p;
    const int pos = proc.scopePos.load();
    for (int i = 0; i < WarpAudioProcessor::scopeSize; ++i)
    {
        const float v = proc.scope[(size_t) ((pos + i) % WarpAudioProcessor::scopeSize)].load();
        const float x = b.getX() + b.getWidth() * (float) i / (float) (WarpAudioProcessor::scopeSize - 1);
        const float y = b.getCentreY() - jlimit (-1.f, 1.f, v) * b.getHeight() * 0.5f * 0.92f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (ui::accent);
    g.strokePath (p, PathStrokeType (1.8f));
}

// ---------------- Editor ----------------
WarpAudioProcessorEditor::WarpAudioProcessorEditor (WarpAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), curve (p), scope (p)
{
    setLookAndFeel (&lnf);

    const char* names[] = { "LOW", "MID", "HIGH" };
    for (int b = 0; b < 3; ++b)
    {
        bandBtn[b].setButtonText (names[b]);
        bandBtn[b].setClickingTogglesState (true);
        bandBtn[b].setRadioGroupId (1);
        bandBtn[b].onClick = [this, b] { selectBand (b); };
        addAndMakeVisible (bandBtn[b]);

        panels[b] = std::make_unique<BandPanel> (p.apvts, b);
        addChildComponent (*panels[b]);
    }

    addAndMakeVisible (curve);
    addAndMakeVisible (scope);

    const std::pair<const char*, const char*> gl[] = {
        { "inGain", "Input" }, { "outGain", "Output" }, { "mix", "Mix" },
        { "xover1", "X-Over 1" }, { "xover2", "X-Over 2" }, { "hp", "HP" },
        { "lp", "LP" }, { "lfoRate", "LFO Rate" }, { "lfoDepth", "LFO Depth" },
        { "envAmt", "Env Amt" }, { "envRel", "Env Rel" } };
    for (auto& [id, name] : gl)
    {
        globals.push_back (std::make_unique<Knob> (p.apvts, id, name));
        addAndMakeVisible (*globals.back());
    }

    bandBtn[0].setToggleState (true, dontSendNotification);
    selectBand (0);
    setSize (900, 612);
}

WarpAudioProcessorEditor::~WarpAudioProcessorEditor() { setLookAndFeel (nullptr); }

void WarpAudioProcessorEditor::selectBand (int b)
{
    for (int i = 0; i < 3; ++i) panels[i]->setVisible (i == b);
    curve.setBand (b);
}

void WarpAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (ui::bg);
    g.setColour (Colours::white);
    g.setFont (FontOptions (26.f, Font::bold));
    g.drawText ("WARP", 16, 10, 200, 36, Justification::centredLeft);
    g.setColour (ui::mute);
    g.setFont (FontOptions (13.f));
    g.drawText ("multiband visual distortion", 120, 10, 300, 36, Justification::centredLeft);

    g.setColour (ui::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat().withTrimmedTop (56.f).reduced (8.f, 4.f), 14.f);
}

void WarpAudioProcessorEditor::resized()
{
    auto b = getLocalBounds().reduced (16);
    b.removeFromTop (40);
    auto top = b.removeFromTop (330);

    curve.setBounds (top.removeFromLeft (320));
    top.removeFromLeft (14);

    auto tabs = top.removeFromTop (30);
    for (int i = 0; i < 3; ++i) bandBtn[i].setBounds (tabs.removeFromLeft (92).reduced (2, 0));
    top.removeFromTop (8);
    for (auto& pnl : panels) pnl->setBounds (top);

    b.removeFromTop (12);
    scope.setBounds (b.removeFromLeft (320));
    b.removeFromLeft (14);

    auto row1 = b.removeFromTop (b.getHeight() / 2);
    const int w = b.getWidth() / 6;
    for (size_t i = 0; i < globals.size(); ++i)
        globals[i]->setBounds (i < 6 ? row1.removeFromLeft (w) : b.removeFromLeft (w));
}
