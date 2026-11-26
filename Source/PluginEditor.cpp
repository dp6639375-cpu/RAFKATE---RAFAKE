#include "PluginEditor.h"
#include <map>

using namespace juce;

// ---------------- BandPanel ----------------
BandPanel::BandPanel (APVTS& s, int band)
{
    const String p = "b" + String (band + 1) + "_";
    for (int i = 0; i < warp::numShapes; ++i) { shapeP.addItem (warp::shapeNames[i], i + 1); shapeN.addItem (warp::shapeNames[i], i + 1); }

    lblP.setText ("Positive", dontSendNotification);
    lblN.setText ("Negative", dontSendNotification);
    for (auto* c : { (Component*) &on, (Component*) &shapeP, (Component*) &shapeN, (Component*) &lblP, (Component*) &lblN })
        addAndMakeVisible (c);

    onAtt = std::make_unique<APVTS::ButtonAttachment>   (s, p + "on", on);
    pAtt  = std::make_unique<APVTS::ComboBoxAttachment> (s, p + "shapeP", shapeP);
    nAtt  = std::make_unique<APVTS::ComboBoxAttachment> (s, p + "shapeN", shapeN);

    drive = std::make_unique<Knob> (s, p + "drive", "Drive");
    bias  = std::make_unique<Knob> (s, p + "bias",  "Bias");
    mix   = std::make_unique<Knob> (s, p + "mix",   "Mix");
    gain  = std::make_unique<Knob> (s, p + "gain",  "Gain");
    for (auto* k : { drive.get(), bias.get(), mix.get(), gain.get() }) addAndMakeVisible (k);
}

void BandPanel::resized()
{
    auto b = getLocalBounds().reduced (12);
    on.setBounds (b.removeFromTop (28).removeFromLeft (120));
    b.removeFromTop (6);
    auto r = b.removeFromTop (28);
    lblP.setBounds (r.removeFromLeft (62));   shapeP.setBounds (r.removeFromLeft (170));
    r.removeFromLeft (16);
    lblN.setBounds (r.removeFromLeft (62));   shapeN.setBounds (r.removeFromLeft (170));
    b.removeFromTop (8);
    const int w = b.getWidth() / 4;
    for (auto* k : { drive.get(), bias.get(), mix.get(), gain.get() })
        k->setBounds (b.removeFromLeft (w));
}

// ---------------- FilterPage ----------------
FilterPage::FilterPage (APVTS& s)
{
    for (int i = 0; i < warp::numFilters; ++i) { preType.addItem (warp::filterNames[i], i + 1); postType.addItem (warp::filterNames[i], i + 1); }
    for (auto* c : { (Component*) &preOn, (Component*) &postOn, (Component*) &preType, (Component*) &postType })
        addAndMakeVisible (c);

    preAtt   = std::make_unique<APVTS::ButtonAttachment>   (s, "pre_on", preOn);
    postAtt  = std::make_unique<APVTS::ButtonAttachment>   (s, "post_on", postOn);
    preTAtt  = std::make_unique<APVTS::ComboBoxAttachment> (s, "pre_type", preType);
    postTAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "post_type", postType);

    const char* names[3] = { "Cutoff", "Resonance", "Morph / Param" };
    const char* ids[3]   = { "cut", "res", "morph" };
    for (int i = 0; i < 3; ++i)
    {
        pre[i]  = std::make_unique<Knob> (s, String ("pre_")  + ids[i], names[i]);
        post[i] = std::make_unique<Knob> (s, String ("post_") + ids[i], names[i]);
        addAndMakeVisible (*pre[i]); addAndMakeVisible (*post[i]);
    }
}

void FilterPage::resized()
{
    auto b = getLocalBounds().reduced (12);
    auto rowH = b.getHeight() / 2;
    auto r1 = b.removeFromTop (rowH), r2 = b;
    auto t1 = r1.removeFromTop (28), t2 = r2.removeFromTop (28);
    preOn.setBounds (t1.removeFromLeft (110));   preType.setBounds (t1.removeFromLeft (190));
    postOn.setBounds (t2.removeFromLeft (110));  postType.setBounds (t2.removeFromLeft (190));
    const int w = r1.getWidth() / 4;
    for (int i = 0; i < 3; ++i) { pre[i]->setBounds (r1.removeFromLeft (w)); post[i]->setBounds (r2.removeFromLeft (w)); }
}

// ---------------- FeedbackPage ----------------
FeedbackPage::FeedbackPage (APVTS& s)
{
    for (auto* t : { &on, &snap, &midi }) addAndMakeVisible (*t);
    onAtt   = std::make_unique<APVTS::ButtonAttachment> (s, "fb_on", on);
    snapAtt = std::make_unique<APVTS::ButtonAttachment> (s, "fb_snap", snap);
    midiAtt = std::make_unique<APVTS::ButtonAttachment> (s, "fb_midi", midi);
    const char* ids[4]   = { "fb_freq", "fb_amt", "fb_damp", "fb_mix" };
    const char* names[4] = { "Pitch", "Amount", "Damping", "Mix" };
    for (int i = 0; i < 4; ++i) { k[i] = std::make_unique<Knob> (s, ids[i], names[i]); addAndMakeVisible (*k[i]); }
}

void FeedbackPage::resized()
{
    auto b = getLocalBounds().reduced (12);
    auto top = b.removeFromTop (28);
    on.setBounds (top.removeFromLeft (120)); snap.setBounds (top.removeFromLeft (130)); midi.setBounds (top.removeFromLeft (130));
    b.removeFromTop (20);
    const int w = b.getWidth() / 4;
    for (auto& kn : k) kn->setBounds (b.removeFromLeft (w));
}

// ---------------- CurveEditor ----------------
CurveEditor::CurveEditor (WarpAudioProcessor& p) : proc (p)
{
    for (int i = 0; i < 8; ++i)
        pts[i] = dynamic_cast<RangedAudioParameter*> (p.apvts.getParameter ("c" + String (i + 1)));
    startTimerHz (30);
}

void CurveEditor::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    { auto inner = ui::drawScreen (g, b); g.reduceClipRegion (inner.toNearestInt()); }
    g.setColour (ui::screenLine);
    for (int i = 1; i < 8; ++i) g.drawVerticalLine ((int) (b.getWidth() * i / 8.f), b.getY(), b.getBottom());

    float v[8];
    for (int i = 0; i < 8; ++i) v[i] = pts[i]->getValue();

    g.setColour (ui::accent.withAlpha (0.25f));
    for (int i = 0; i < 8; ++i)
    {
        const float h = v[i] * (b.getHeight() - 6.f);
        g.fillRect (b.getWidth() * i / 8.f + 2.f, b.getBottom() - h - 3.f, b.getWidth() / 8.f - 4.f, h);
    }

    Path p;
    for (int k = 0; k <= 200; ++k)
    {
        const float ph = (float) k / 201.f;
        const float y = b.getBottom() - 3.f - warp::curveValue (v, ph) * (b.getHeight() - 6.f);
        const float x = b.getWidth() * ph;
        if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setGradientFill (ColourGradient (ui::accent, 0.f, 0.f, ui::accent2, b.getWidth(), 0.f, false));
    g.strokePath (p, PathStrokeType (2.5f));

    g.setColour (Colours::white.withAlpha (0.8f));
    g.drawVerticalLine ((int) (b.getWidth() * proc.curvePhase.load()), b.getY(), b.getBottom());
}

void CurveEditor::setFrom (const MouseEvent& e)
{
    const int i = jlimit (0, 7, (int) (e.position.x / (float) getWidth() * 8.f));
    if (i != active)
    {
        if (active >= 0) pts[active]->endChangeGesture();
        active = i;
        pts[active]->beginChangeGesture();
    }
    pts[i]->setValueNotifyingHost (1.f - jlimit (0.f, 1.f, e.position.y / (float) getHeight()));
}

void CurveEditor::mouseDown (const MouseEvent& e) { setFrom (e); }
void CurveEditor::mouseDrag (const MouseEvent& e) { setFrom (e); }
void CurveEditor::mouseUp (const MouseEvent&)
{
    if (active >= 0) pts[active]->endChangeGesture();
    active = -1;
}

// ---------------- ModPage ----------------
ModPage::ModPage (WarpAudioProcessor& p) : editor (p)
{
    addAndMakeVisible (editor);
    auto& s = p.apvts;
    auto addLabel = [this] (const String& t)
    {
        auto* l = labels.add (new Label ({}, t));
        addAndMakeVisible (l);
    };
    for (auto* t : { "", "Rate", "Drive", "Cutoff", "Feedback" }) addLabel (t);

    const char* src[5]  = { "LFO", "Curve", "Follow", "Macro 1", "Macro 2" };
    const char* sid[5]  = { "lfo", "crv", "env", "m1", "m2" };
    const char* rid[3]  = { "lfoRate", "crvRate", "envRel" };
    const char* tid[3]  = { "drive", "cut", "fb" };
    for (int r = 0; r < 5; ++r)
    {
        addLabel (src[r]);
        if (r < 3) { rate[r] = std::make_unique<HSlider> (s, rid[r]); addAndMakeVisible (*rate[r]); }
        for (int t = 0; t < 3; ++t)
        {
            amt[r][t] = std::make_unique<HSlider> (s, String ("mod_") + sid[r] + "_" + tid[t]);
            addAndMakeVisible (*amt[r][t]);
        }
    }
}

void ModPage::resized()
{
    auto b = getLocalBounds().reduced (8, 6);
    editor.setBounds (b.removeFromTop (100));
    b.removeFromTop (6);
    const int labelW = 62, colW = (b.getWidth() - labelW) / 4;

    auto head = b.removeFromTop (18);
    head.removeFromLeft (labelW);
    for (int c = 0; c < 4; ++c) labels[c + 1]->setBounds (head.removeFromLeft (colW));

    for (int r = 0; r < 5; ++r)
    {
        auto row = b.removeFromTop (30);
        labels[5 + r]->setBounds (row.removeFromLeft (labelW));
        auto rateCell = row.removeFromLeft (colW);
        if (r < 3) rate[r]->setBounds (rateCell.reduced (2, 3));
        for (int t = 0; t < 3; ++t) amt[r][t]->setBounds (row.removeFromLeft (colW).reduced (2, 3));
    }
}

// ---------------- CurveView ----------------
void CurveView::paint (Graphics& g)
{
    auto b = ui::drawScreen (g, getLocalBounds().toFloat().reduced (2.f));

    g.setColour (ui::screenLine);
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
    const int   tP = (int) val ("shapeP"), tN = (int) val ("shapeN");
    const bool  active = val ("on") > 0.5f;

    auto toX = [&] (float x) { return b.getX() + (x + 1.f) * 0.5f * b.getWidth(); };
    auto toY = [&] (float y) { return b.getBottom() - (jlimit (-1.f, 1.f, y) + 1.f) * 0.5f * b.getHeight(); };

    Path p;
    for (int k = 0; k <= 256; ++k)
    {
        const float x = -1.f + 2.f * (float) k / 256.f;
        const float v = x * driveG + bias;
        const float y = active ? warp::shape (v, v >= 0.f ? tP : tN) : x;
        if (k == 0) p.startNewSubPath (toX (x), toY (y)); else p.lineTo (toX (x), toY (y));
    }
    g.setGradientFill (ColourGradient (ui::accent, b.getX(), b.getBottom(), ui::accent2, b.getRight(), b.getY(), false));
    g.strokePath (p, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));

    if (active)
    {
        const float dx = toX (proc.bandIn[band].load()), dy = toY (proc.bandOut[band].load());
        g.setColour (ui::accent2.withAlpha (0.3f));  g.fillEllipse (dx - 11.f, dy - 11.f, 22.f, 22.f);
        g.setColour (Colours::white);             g.fillEllipse (dx - 4.f,  dy - 4.f,  8.f,  8.f);
    }
}

// ---------------- ScopeView (oscilloscope + multiband spectrum) ----------------
ScopeView::ScopeView (WarpAudioProcessor& p) : proc (p)
{
    smooth.fill (-100.f);
    startTimerHz (30);
}

void ScopeView::timerCallback()
{
    if (mode == 1) computeSpectrum();
    repaint();
}

void ScopeView::computeSpectrum()
{
    const int pos = proc.fftPos.load();
    for (int i = 0; i < 2048; ++i)
        data[(size_t) i] = proc.fftRing[(size_t) ((pos + 2048 + i) & 4095)].load();
    std::fill (data.begin() + 2048, data.end(), 0.f);
    window.multiplyWithWindowingTable (data.data(), 2048);
    fft.performFrequencyOnlyForwardTransform (data.data());
    for (int k = 0; k < 1024; ++k)
    {
        const float db = jlimit (-100.f, 6.f, Decibels::gainToDecibels (data[(size_t) k] / (2048.f * 0.25f), -100.f));
        smooth[(size_t) k] = jmax (db, smooth[(size_t) k] - 1.5f);
    }
}

void ScopeView::paint (Graphics& g)
{
    auto b = ui::drawScreen (g, getLocalBounds().toFloat().reduced (2.f));

    if (mode == 0)
    {
        g.setColour (ui::screenLine);
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
    else
    {
        const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0;
        auto fx = [&] (float f) { return b.getX() + b.getWidth() * std::log (f / 20.f) / std::log (1000.f); };
        const float x1 = proc.apvts.getRawParameterValue ("xover1")->load();
        const float x2 = jmax (proc.apvts.getRawParameterValue ("xover2")->load(), x1 * 1.25f);
        const float px1 = fx (x1), px2 = fx (x2);

        const Colour bandCol[3] = { ui::accent, ui::accent2, Colour (0xff35c7a5) };
        const float edges[4] = { b.getX(), px1, px2, b.getRight() };
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (bandCol[i].withAlpha (0.10f));
            g.fillRect (edges[i], b.getY(), edges[i + 1] - edges[i], b.getHeight());
        }

        Path p;
        const int w = (int) b.getWidth();
        for (int px = 0; px <= w; ++px)
        {
            const float f = 20.f * std::pow (1000.f, (float) px / (float) w);
            const float bin = jlimit (0.f, 1022.f, f * 2048.f / (float) sr);
            const int i0 = (int) bin;
            const float db = smooth[(size_t) i0] + (smooth[(size_t) i0 + 1] - smooth[(size_t) i0]) * (bin - (float) i0);
            const float y = b.getBottom() - jlimit (0.f, 1.f, (db + 90.f) / 90.f) * b.getHeight();
            const float x = b.getX() + (float) px;
            if (px == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        Path fill (p);
        fill.lineTo (b.getRight(), b.getBottom()); fill.lineTo (b.getX(), b.getBottom()); fill.closeSubPath();
        g.setGradientFill (ColourGradient (ui::accent.withAlpha (0.35f), 0.f, b.getY(), ui::accent.withAlpha (0.02f), 0.f, b.getBottom(), false));
        g.fillPath (fill);
        g.setColour (ui::accent);
        g.strokePath (p, PathStrokeType (1.6f));

        g.setColour (ui::screenText.withAlpha (0.6f));
        g.drawVerticalLine ((int) px1, b.getY(), b.getBottom());
        g.drawVerticalLine ((int) px2, b.getY(), b.getBottom());
        g.setFont (FontOptions (10.f));
        g.drawText ("LOW",  (int) edges[0] + 4, (int) b.getY() + 3, 40, 12, Justification::left);
        g.drawText ("MID",  (int) px1 + 4, (int) b.getY() + 3, 40, 12, Justification::left);
        g.drawText ("HIGH", (int) px2 + 4, (int) b.getY() + 3, 40, 12, Justification::left);
    }
    g.setColour (ui::screenText.withAlpha (0.7f));
    g.setFont (FontOptions (10.f));
    g.drawText (mode == 1 ? "spectrum - click: scope" : "scope - click: spectrum",
                getLocalBounds().removeFromBottom (16).reduced (8, 0), Justification::centredRight);
}

// ---------------- Editor ----------------
WarpAudioProcessorEditor::WarpAudioProcessorEditor (WarpAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), curve (p), scope (p), filt (p.apvts), fb (p.apvts), mod (p)
{
    ui::setDark ((bool) proc.apvts.state.getProperty ("dark", true));
    lnf.apply();
    setLookAndFeel (&lnf);
    themeBtn.setButtonText (ui::isDark ? "Silver" : "Graphite");

    const char* pages[] = { "DISTORTION", "FILTERS", "FEEDBACK", "MODULATION" };
    for (int i = 0; i < 4; ++i)
    {
        pageBtn[i].setButtonText (pages[i]);
        pageBtn[i].setClickingTogglesState (true);
        pageBtn[i].setRadioGroupId (2);
        pageBtn[i].onClick = [this, i] { setPage (i); };
        addAndMakeVisible (pageBtn[i]);
    }

    const char* names[] = { "LOW", "MID", "HIGH" };
    for (int i = 0; i < 3; ++i)
    {
        bandBtn[i].setButtonText (names[i]);
        bandBtn[i].setClickingTogglesState (true);
        bandBtn[i].setRadioGroupId (1);
        bandBtn[i].onClick = [this, i] { selectBand (i); };
        addAndMakeVisible (bandBtn[i]);

        panels[i] = std::make_unique<BandPanel> (p.apvts, i);
        addChildComponent (*panels[i]);
    }
    addChildComponent (filt); addChildComponent (fb); addChildComponent (mod);
    addAndMakeVisible (curve);
    addAndMakeVisible (scope);

    buildPresetMenu();
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (+1); };
    addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn);

    randomBtn.onClick = [this] { proc.randomize(); };
    addAndMakeVisible (randomBtn);

    playBtn.setClickingTogglesState (true);
    playBtn.onClick = [this] { playMode = playBtn.getToggleState(); updateVisibility(); resized(); repaint(); };
    addAndMakeVisible (playBtn);

    themeBtn.onClick = [this] { applyTheme (! ui::isDark); };
    addAndMakeVisible (themeBtn);

    const std::pair<const char*, const char*> gl[] = {
        { "inGain", "Input" }, { "outGain", "Output" }, { "mix", "Mix" },
        { "xover1", "X-Over 1" }, { "xover2", "X-Over 2" }, { "m1", "Macro 1" }, { "m2", "Macro 2" } };
    for (auto& [id, name] : gl)
    {
        globals.push_back (std::make_unique<Knob> (p.apvts, id, name));
        addAndMakeVisible (*globals.back());
    }
    bigMacro[0] = std::make_unique<Knob> (p.apvts, "m1", "Macro 1");
    bigMacro[1] = std::make_unique<Knob> (p.apvts, "m2", "Macro 2");
    for (auto& m : bigMacro) addChildComponent (*m);

    bandBtn[0].setToggleState (true, dontSendNotification);
    pageBtn[0].setToggleState (true, dontSendNotification);
    setSize (960, 604);
    updateVisibility();
}

WarpAudioProcessorEditor::~WarpAudioProcessorEditor() { setLookAndFeel (nullptr); }

void WarpAudioProcessorEditor::applyTheme (bool dark)
{
    ui::setDark (dark);
    proc.apvts.state.setProperty ("dark", dark, nullptr);
    lnf.apply();
    themeBtn.setButtonText (dark ? "Silver" : "Graphite");
    sendLookAndFeelChange();
    repaint();
}

void WarpAudioProcessorEditor::buildPresetMenu()
{
    presetBox.clear (dontSendNotification);
    auto* root = presetBox.getRootMenu();
    std::map<String, PopupMenu> cats;
    std::vector<String> order;
    for (int i = 0; i < proc.getNumPrograms(); ++i)
    {
        const String name = proc.getProgramName (i);
        const int sp = name.lastIndexOfChar (' ');
        const String cat = (sp > 0 && name.substring (sp + 1).containsOnly ("0123456789")) ? name.substring (0, sp) : String ("Factory");
        if (cats.find (cat) == cats.end()) order.push_back (cat);
        cats[cat].addItem (i + 1, name);
    }
    for (auto& c : order) root->addSubMenu (c, cats[c]);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.setText (proc.getProgramName (proc.getCurrentProgram()), dontSendNotification);
}

void WarpAudioProcessorEditor::stepPreset (int delta)
{
    const int n = proc.getNumPrograms();
    const int next = (proc.getCurrentProgram() + delta + n) % n;
    proc.setCurrentProgram (next);
    presetBox.setSelectedId (next + 1, dontSendNotification);
    presetBox.setText (proc.getProgramName (next), dontSendNotification);
}

void WarpAudioProcessorEditor::selectBand (int b) { band = b; curve.setBand (b); updateVisibility(); }

void WarpAudioProcessorEditor::setPage (int p) { page = p; updateVisibility(); resized(); }

void WarpAudioProcessorEditor::updateVisibility()
{
    const bool adv = ! playMode;
    for (int i = 0; i < 4; ++i) pageBtn[i].setVisible (adv);
    for (int i = 0; i < 3; ++i)
    {
        bandBtn[i].setVisible (adv && page == 0);
        panels[i]->setVisible (adv && page == 0 && i == band);
    }
    filt.setVisible (adv && page == 1);
    fb.setVisible (adv && page == 2);
    mod.setVisible (adv && page == 3);
    for (auto& m : bigMacro) m->setVisible (playMode);
    for (size_t i = 0; i < globals.size(); ++i) globals[i]->setVisible (adv || i < 3);
}

static Image makeChassis (int w, int h)
{
    Image img (Image::ARGB, w, h, true);
    Graphics g (img);
    const float fw = (float) w, fh = (float) h;

    g.setGradientFill (ColourGradient (ui::chassisTop, 0.f, 0.f, ui::chassisBottom, 0.f, fh, false));
    g.fillAll();

    Random rnd (7);                                           // brushed metal grain
    for (int y = 0; y < h; y += 2)
    {
        g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (0.02f + rnd.nextFloat() * 0.035f));
        g.drawHorizontalLine (y, 0.f, fw);
    }

    g.setGradientFill (ColourGradient (Colours::white.withAlpha (ui::isDark ? 0.10f : 0.45f), 0.f, 0.f,
                                       Colours::white.withAlpha (0.f), 0.f, fh * 0.22f, false));
    g.fillRect (0.f, 0.f, fw, fh * 0.22f);                    // aero glass sheen

    ColourGradient vig (Colours::transparentBlack, fw * 0.5f, fh * 0.5f,
                        Colours::black.withAlpha (ui::isDark ? 0.65f : 0.50f), 0.f, 0.f, true);
    vig.addColour (0.55, Colours::transparentBlack);          // darkened borders
    g.setGradientFill (vig);
    g.fillAll();

    g.setColour (Colours::white.withAlpha (0.45f));           // chassis bevel
    g.drawLine (0.f, 1.f, fw, 1.f, 2.f);  g.drawLine (1.f, 0.f, 1.f, fh, 2.f);
    g.setColour (Colours::black.withAlpha (0.7f));
    g.drawLine (0.f, fh - 1.f, fw, fh - 1.f, 2.f);  g.drawLine (fw - 1.f, 0.f, fw - 1.f, fh, 2.f);

    ui::drawScrew (g, 7.f, 7.f, 0.5f);          ui::drawScrew (g, fw - 7.f, 7.f, 2.1f);
    ui::drawScrew (g, 7.f, fh - 7.f, 1.2f);     ui::drawScrew (g, fw - 7.f, fh - 7.f, 0.2f);
    return img;
}

void WarpAudioProcessorEditor::paint (Graphics& g)
{
    if (! chassis.isValid() || chassis.getWidth() != getWidth() || chassis.getHeight() != getHeight() || chassisDark != ui::isDark)
    {
        chassis = makeChassis (getWidth(), getHeight());
        chassisDark = ui::isDark;
    }
    g.drawImageAt (chassis, 0, 0);

    // brand plate: RAF-KATE / By: Zacci Records
    Rectangle<float> plate (16.f, 6.f, 244.f, 40.f);
    g.setGradientFill (ColourGradient (Colour (0xff3a3f45), plate.getX(), plate.getY(), Colour (0xff0b0c0e), plate.getX(), plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 3.f);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.20f), plate.getX(), plate.getY(), Colours::white.withAlpha (0.02f), plate.getX(), plate.getCentreY(), false));
    g.fillRoundedRectangle (plate.reduced (1.f).withHeight (plate.getHeight() * 0.5f), 3.f);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (plate, 3.f, 1.2f);

    g.setFont (FontOptions (23.f, Font::bold));
    g.setColour (Colours::black.withAlpha (0.8f));
    g.drawText ("RAF-KATE", plate.reduced (12.f, 0.f).translated (1.f, 1.f).withHeight (28.f), Justification::centredLeft);
    g.setColour (Colour (0xffeef2f5));
    g.drawText ("RAF-KATE", plate.reduced (12.f, 0.f).withHeight (28.f), Justification::centredLeft);
    g.setFont (FontOptions (10.5f, Font::italic));
    g.setColour (Colour (0xff8fc4ea));
    g.drawText ("By: Zacci Records", plate.reduced (13.f, 0.f).withTrimmedTop (26.f), Justification::centredLeft);

    ui::drawPanel (g, getLocalBounds().toFloat().withTrimmedTop (54.f).reduced (12.f, 6.f));
}

void WarpAudioProcessorEditor::resized()
{
    auto b = getLocalBounds().reduced (16);
    auto header = b.removeFromTop (40);
    themeBtn.setBounds (header.removeFromRight (72).reduced (0, 6));  header.removeFromRight (6);
    playBtn.setBounds  (header.removeFromRight (64).reduced (0, 6));  header.removeFromRight (6);
    randomBtn.setBounds (header.removeFromRight (96).reduced (0, 6)); header.removeFromRight (10);
    nextBtn.setBounds (header.removeFromRight (30).reduced (0, 6));
    presetBox.setBounds (header.removeFromRight (190).reduced (0, 6));
    prevBtn.setBounds (header.removeFromRight (30).reduced (0, 6));

    auto top = b.removeFromTop (330);
    curve.setBounds (top.removeFromLeft (320));
    top.removeFromLeft (14);

    if (playMode)
    {
        bigMacro[0]->setBounds (top.removeFromLeft (top.getWidth() / 2));
        bigMacro[1]->setBounds (top);
    }
    else
    {
        auto tabs = top.removeFromTop (28);
        for (int i = 0; i < 4; ++i) pageBtn[i].setBounds (tabs.removeFromLeft (122).reduced (2, 0));
        top.removeFromTop (6);

        auto body = top;
        if (page == 0)
        {
            auto bandRow = body.removeFromTop (30);
            for (int i = 0; i < 3; ++i) bandBtn[i].setBounds (bandRow.removeFromLeft (92).reduced (2, 0));
            body.removeFromTop (4);
        }
        for (auto& pnl : panels) pnl->setBounds (body);
        filt.setBounds (body); fb.setBounds (body); mod.setBounds (top);
    }

    b.removeFromTop (12);
    scope.setBounds (b.removeFromLeft (320));
    b.removeFromLeft (14);
    const int shown = playMode ? 3 : (int) globals.size();
    const int w = b.getWidth() / shown;
    for (int i = 0; i < shown; ++i) globals[(size_t) i]->setBounds (b.removeFromLeft (w));
}
