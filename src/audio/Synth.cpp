/***************************************************************************

Copyright (C) 2016  stefan.berke @ modular-audio-graphics.com

This source is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this software; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

****************************************************************************/

#include "Synth.h"

#if (0)
#   define SONOT_DEBUG_SYNTH(arg__) qDebug() << arg__
#else
#   define SONOT_DEBUG_SYNTH(unused__)
#endif


namespace Sonot {

// ------------------------------------ synthvoice private ------------------------------------

class SynthVoice::Private
{
    public:

    Private(Synth * synth)
        : synth     (synth),
          active	(false),
          cued      (false),
          cuedForStop(false),
          note   	(0),
          index     (0),
          startSample(0),
          stopSample(0),
          freq		(0.0),
          velo		(0.0),
          fenvAmt   (0),
          lifetime	(0),
          nextUnison(0),
          data      (0)

    { }

    double curLevel() const { return velo * env.value(); }

    Synth * synth;

    bool active, cued, cuedForStop;

    int note;
    size_t index, startSample, stopSample;

    double
        freq,
        velo,
        fenvAmt;

    std::vector<double>
        freq_c,
        phase;

    size_t lifetime;

    EnvelopeGenerator<double> env;

    SynthVoice * nextUnison;

    void * data;
};




// ------------------------------------ synthvoice ------------------------------------

SynthVoice::SynthVoice(Synth * synth)
    : p_    (new Private(synth))
{
}

SynthVoice::~SynthVoice()
{
    delete p_;
}

Synth * SynthVoice::synth() const { return p_->synth; }
size_t SynthVoice::index() const { return p_->index; }
bool SynthVoice::active() const { return p_->active; }
bool SynthVoice::cued() const { return p_->cued; }
int SynthVoice::note() const { return p_->note; }
size_t SynthVoice::startSample() const { return p_->startSample; }
double SynthVoice::frequency() const { return p_->freq; }
double SynthVoice::phase() const { return p_->phase[0]; }
double SynthVoice::velocity() const { return p_->velo; }
double SynthVoice::attack() const { return p_->env.attack(); }
double SynthVoice::decay() const { return p_->env.decay(); }
double SynthVoice::sustain() const { return p_->env.sustain(); }
double SynthVoice::release() const { return p_->env.release(); }
const EnvelopeGenerator<double>& SynthVoice::envelope() const { return p_->env; }
SynthVoice * SynthVoice::nextUnisonVoice() const { return p_->nextUnison; }
void * SynthVoice::userData() const { return p_->data; }

void SynthVoice::setUserData(void *data) { p_->data = data; }







// -------------------------- synth private ------------------------------------


class Synth::Private
{
public:
    Private(Synth * s)
        : p             (s),
          voicePolicy   (Synth::VP_QUITEST),
          sampleRate    (44100),
          props         ("synth"),
          cbStart_      (0),
          cbEnd_        (0)
    {
        createProperties();
    }

    ~Private()
    {
        deleteVoices();
    }

    void createProperties();

    void deleteVoices()
    {
        for (auto v : voices)
        {
            if (v->p_->active && p->p_->cbEnd_)
                p->p_->cbEnd_(v);
            delete v;
        }
        voices.clear();
    }

    void setNumVoices(size_t n)
    {
        deleteVoices();
        voices.resize(n);
        for (size_t i=0; i<n; ++i)
        {
            voices[i] = new SynthVoice(p);
            voices[i]->p_->index = i;
        }
    }

    SynthVoice * noteOn(
            size_t startSample, double freq, int note, double velocity,
            size_t numCombinedUnison, void *userData);
    void noteOff(size_t stopSample, int note);
    void panic();
    /** Mono output */
    void process(float * output, size_t bufferLength);
    /** Multichannel output */
    void process(float ** output, size_t bufferLength);

    Synth * p;

    std::vector<SynthVoice*> voices;

    Synth::VoicePolicy voicePolicy;

    size_t sampleRate;

    QProps::Properties props;

    NoteFreq<double> noteFreq;

    std::function<void(SynthVoice*)>
        cbStart_, cbEnd_;
};


QProps::Properties::NamedValues Synth::voicePolicyNamedValues()
{
    QProps::Properties::NamedValues nv;
    nv.set("forget", tr("forget"),
        tr("Don't start a new voice when maximum polyphony is reached"),
           (int)VP_FORGET);
    nv.set("lowest", tr("lowest"),
        tr("Stop and reuse the lowest voice when maximum polyphony is reached"),
           (int)VP_LOWEST);
    nv.set("highest", tr("highest"),
        tr("Stop and reuse the highest voice when maximum polyphony is reached"),
           (int)VP_HIGHEST);
    nv.set("oldest", tr("oldest"),
        tr("Stop and reuse the oldest voice when maximum polyphony is reached"),
           (int)VP_OLDEST);
    nv.set("newest", tr("newest"),
        tr("Stop and reuse the newest voice when maximum polyphony is reached"),
           (int)VP_NEWEST);
    nv.set("quitest", tr("quitest"),
        tr("Stop and reuse the quitest voice when maximum polyphony is reached"),
           (int)VP_QUITEST);
    nv.set("loudest", tr("loudest"),
        tr("Stop and reuse the loudest voice when maximum polyphony is reached"),
           (int)VP_LOUDEST);
    return nv;
}

void Synth::Private::createProperties()
{
    props.set("number-voices", tr("number voices"),
              tr("The number of polyphonic voices"),
              24);
    props.setRange("number-voices", 0, 1024);

    props.set("voice-policy", tr("voice reuse policy"),
              tr("Sets the policy to apply when the maximum polyphony "
                 "is reached and a new note-on is requested"),
              voicePolicyNamedValues(), (int)VP_OLDEST);

    props.set("volume", tr("master volume"),
              tr("Master volume of all played voices"),
              1.);
    props.setMin("volume", 0.);
    props.setStep("volume", 0.01);

    props.set("number-unisono-voices", tr("unisono voices"),
              tr("The number of unisono voices that will be played "
                 "for one note"),
              1);
    props.setRange("number-unisono-voices", 1, 512);

    props.set("real-unisono", tr("real unisono voices"),
              tr("Should unisono voices be individual synthesizer voices "
                 "or should they be combined into one."),
              true);

    props.set("unisono-note-step", tr("unisono note step"),
              tr("Note to be added/subtracted for each unisono voice"),
              0);

    props.set("unisono-detune", tr("uinsono-detune"),
              tr("The amount of random detuning for each individual "
                 "unisono voice in cents (100 per full note)"),
              11.);

    props.set("base-freq", tr("base frequency"),
              tr("The frequency in Hertz of the lowest C note"),
              noteFreq.baseFrequency());

    props.set("notes-per-octave", tr("notes per octave"),
              tr("The number of notes per one octave"),
              noteFreq.notesPerOctave());
    props.setMin("notes-per-octave", 1./1000.);

    props.set("attack", tr("attack"),
              tr("Attack time of envelope in seconds"),
              0.02, 0., 10000., 0.01);

    props.set("decay", tr("decay"),
              tr("Decay time of envelope in seconds"),
              1., 0., 10000., 0.01);

    props.set("sustain", tr("sustain"),
              tr("Sustain level of envelope"),
              0.0);
    props.setStep("sustain", 0.01);

    props.set("release", tr("release"),
              tr("Release time of envelope in seconds"),
              1., 0., 10000., 0.01);
}


SynthVoice * Synth::Private::noteOn(
        size_t startSample, double freq, int note, double velocity,
        size_t numCombinedUnison, void * userData)
{
    if (voices.empty())
        return 0;

    // find free (non-active & non-cued) voice
    auto i = voices.begin();
    for (; i!=voices.end(); ++i)
        if (!(*i)->p_->active && !(*i)->p_->cued) break;

    // none free?
    if (i == voices.end())
    {
        if (voicePolicy == Synth::VP_FORGET)
            return nullptr;

        SONOT_DEBUG_SYNTH("Synth::noteOn() looking for voice to reuse");

        i = voices.begin();

        // policy doesn't matter for monophonic synth
        if (voices.size()>1)
        {
            // decide which to overwrite

            // this macro compares the value__s of each voice
            // and returns the voice to reuse in 'i'
            // it also checks for the envelope state and
            // rather reuses voices that are at a later state
            #define MO__FIND_VOICE_ENV(value__, operator__)                 \
            {                                                               \
                auto j = i;                                                 \
                auto val1 = (*i)->p_->value__;                              \
                auto val2 = val1;                                           \
                ++j;                                                        \
                for (; j != voices.end(); ++j)                              \
                {                                                           \
                    auto val = (*j)->p_->value__;                           \
                    if (val operator__ val1                                 \
                        && (*j)->p_->env.state() > (*i)->p_->env.state())   \
                    {                                                       \
                        val1 = val;                                         \
                        i = j;                                              \
                    }                                                       \
                    else if (val operator__ val2)                           \
                    {                                                       \
                        val2 = val;                                         \
                        i = j;                                              \
                    }                                                       \
                }                                                           \
            }

            // same as above but does not care for envelope states
            #define MO__FIND_VOICE_NO_ENV(value__, operator__)              \
            {                                                               \
                auto j = i;                                                 \
                auto val1 = (*i)->p_->value__;                              \
                ++j;                                                        \
                for (; j != voices.end(); ++j)                              \
                {                                                           \
                    auto val = (*j)->p_->value__;                           \
                    f (val operator__ val1)                                 \
                    {                                                       \
                        val1 = val;                                         \
                        i = j;                                              \
                    }                                                       \
                }                                                           \
            }

            // XXX place for branching with a yet missing option in Synth
            #define MO__FIND_VOICE(value__, operator__) \
                    MO__FIND_VOICE_ENV(value__, operator__)

            switch (voicePolicy)
            {
                case Synth::VP_LOWEST:
                    MO__FIND_VOICE(freq, <);
                break;

                case Synth::VP_HIGHEST:
                    MO__FIND_VOICE(freq, >);
                break;

                case Synth::VP_OLDEST:
                    MO__FIND_VOICE(lifetime, >);
                break;

                case Synth::VP_NEWEST:
                    MO__FIND_VOICE(lifetime, <);
                break;

                case Synth::VP_QUITEST:
                    MO__FIND_VOICE(curLevel(), <);
                break;

                case Synth::VP_LOUDEST:
                    MO__FIND_VOICE(curLevel(), >);
                break;

                case Synth::VP_FORGET: break;
            }

            #undef MO__FIND_VOICE
            #undef MO__FIND_VOICE_ENV
            #undef MO__FIND_VOICE_NO_ENV
        }

        SONOT_DEBUG_SYNTH("Synth::noteOn(): reusing voice " << (*i)->p_->index);
    }

    // (re-)init voice

    SynthVoice::Private * v = (*i)->p_;

    SONOT_DEBUG_SYNTH("init voice " << v->index << " f="
                      << freq << "hz " << " v=" << velocity);

    v->lifetime = 0;
    v->active = false;
    v->cued = true;
    v->cuedForStop = false;
    v->note = note;

    v->freq = freq;

    // freq as coefficient
    const double freqc = v->freq / sampleRate;
    v->freq_c.resize(numCombinedUnison);
    v->phase.resize(numCombinedUnison);
    for (size_t i=0; i<numCombinedUnison; ++i)
    {
        v->freq_c[i] = freqc;
        v->phase[i] = 0.0;
    }
    v->velo = velocity;
    v->startSample = startSample;
    v->stopSample = 0;
    v->env.setSampleRate(sampleRate);
    v->env.setAttack(p->attack());
    v->env.setDecay(p->decay());
    v->env.setSustain(p->sustain());
    v->env.setRelease(p->release());
    v->data = userData;
    v->nextUnison = nullptr;

    return *i;
}

void Synth::Private::noteOff(size_t stopSample, int note)
{
    for (SynthVoice* i : voices)
    if (i->p_->note == note
        && (i->p_->active || (i->p_->cued && i->p_->startSample <= stopSample))
        )
    {
        i->p_->cuedForStop = true;
        i->p_->stopSample = stopSample;
    }
}

void Synth::Private::panic()
{
    for (auto i : voices)
    {
        i->p_->active = i->p_->cued = false;
    }
}

void Synth::Private::process(float *output, size_t bufferLength)
{
    memset(output, 0, sizeof(float) * bufferLength);

    const double vol = p->volume();

    // for each sample
    for (size_t sample = 0; sample < bufferLength; ++sample, ++output)
    {
        // for each voice
        for (SynthVoice * i : voices)
        {
            SynthVoice::Private * v = i->p_;

            // cued for stop
            if (v->cuedForStop && v->stopSample == sample)
            {
                //qDebug() << "STOP " << v->note;
                v->cuedForStop = false;
                if (v->env.release() > 0.)
                {
                    v->env.setState(ENV_RELEASE);
                }
                else
                {
                    v->env.stop();
                    v->active = false;
                    if (cbEnd_)
                        cbEnd_(i);
                    continue;
                }
            }

            // start cued voice
            if (v->cued && v->startSample == sample)
            {
                //qDebug() << "START " << v->note;
                v->env.trigger();
                v->active = true;
                v->cued = false;
                if (cbStart_)
                    cbStart_(i);
            }

            if (!v->active)
                continue;

            // count number of samples alive
            ++(v->lifetime);

            float s = 0.0;
            for (size_t j = 0; j<v->phase.size(); ++j)
            {
                // advance phase counter
                v->phase[j] += v->freq_c[j];
                // get sample
                s += std::sin(v->phase[j] * 3.14159265 * 2.);
            }

            // put into buffer
            *output += s * vol * v->velo * v->env.value();

            // process envelope
            v->env.next();
            // check for end of envelope
            if (!v->env.active())
            {
                v->active = false;
                if (cbEnd_)
                    cbEnd_(i);
                continue;
            }
        }
    }
}

void Synth::Private::process(float ** outputs, size_t bufferLength)
{
    const double vol = p->volume();

    // for each voice
    for (size_t voicenum = 0; voicenum < voices.size(); ++voicenum)
    {
        float * output = outputs[voicenum];
        if (!output)
            continue;

        SynthVoice::Private * v = voices[voicenum]->p_;

        // if voice is inactive (and won't become active)
        if (!v->active && !v->cued)
        {
            //SONOT_DEBUG_SYNTH("unused " << v->index);

            // clear output buffer
            memset(output, 0, sizeof(float) * bufferLength);

            continue;
        }

        // where to start rendering
        size_t start = 0;

        // start voice?
        if (v->cued)
        {
            if (v->startSample < bufferLength)
            {
                SONOT_DEBUG_SYNTH("start cued voice " << v->index << " s=" << v->startSample);

                // start at specified start-sample
                start = v->startSample;
                v->env.trigger();
                v->active = true;
                v->cued = false;
                // send callback
                if (cbStart_)
                    cbStart_(voices[voicenum]);

                // clear first part of buffer
                memset(output, 0, sizeof(float) * start);
                output += start;
            }
            else
            {
                // if startsample is out of range
                // don't check again
                v->cued = false;
                continue;
            }
        }

        //MO_DEBUG("process " << v->index << " " << start << "-" << bufferLength);

        // for each sample
        for (size_t sample = start; sample < bufferLength; ++sample, ++output)
        {
            // get oscillator sample
            float s = 0.0;
            for (size_t j = 0; j<v->phase.size(); ++j)
            {
                // advance phase counter
                v->phase[j] += v->freq_c[j];
                // get sample
                s += std::sin(v->phase[j] * 3.14159265 * 2.);
            }

            // envelope before filter
            s *= v->env.value();

            // put into buffer
            *output = s * vol * v->velo;

            // process envelope
            v->env.next();
            // check for end of envelope
            if (!v->env.active())
            {
                SONOT_DEBUG_SYNTH("voice end " << v->index << " s=" << sample);

                v->active = false;
                // clear rest of buffer
                if (sample < bufferLength - 1)
                    memset(output+1, 0, sizeof(float) * (bufferLength - sample - 1));
                // send callback
                if (cbEnd_)
                    cbEnd_(voices[voicenum]);
                break;
            }

        }

        // count number of samples alive
        v->lifetime += bufferLength - start;
    }
}




// -------------------------------- synth ------------------------------------


Synth::Synth()
    : p_    (new Private(this))
{
    p_->setNumVoices(numberVoices());
}

Synth::~Synth()
{
    delete p_;
}

size_t Synth::sampleRate() const { return p_->sampleRate; }

const QProps::Properties& Synth::props() const { return p_->props; }

void Synth::setProperties(const QProps::Properties& p)
{
    p_->props = p;
    p_->noteFreq.setBaseFrequency( baseFreq() );
    p_->noteFreq.setNotesPerOctave( notesPerOctave() );
    if (numberVoices() != p_->voices.size())
        p_->setNumVoices(numberVoices());
}

void Synth::setVoiceStartedCallback(std::function<void (SynthVoice *)> func) { p_->cbStart_ = func; }
void Synth::setVoiceEndedCallback(std::function<void (SynthVoice *)> func) { p_->cbEnd_ = func; }



SynthVoice * Synth::noteOn(int note, double velocity,
                           size_t startSample, void * userData)
{
    SONOT_DEBUG_SYNTH("Synth::noteOn(" << note << ", " << velocity << ", "
             << startSample << ", " << userData << ")");

    double freq = p_->noteFreq.frequency(note);

    SynthVoice * voice = p_->noteOn(startSample, freq, note, velocity,
                                    combinedUnison() ? unisonVoices() : 1,
                                    userData);
    if (!voice)
    {
        SONOT_DEBUG_SYNTH("Synth::noteOn() failed");
        return 0;
    }

    // no unisono mode?
    if (unisonVoices() < 2)
        return voice;

    // set frequency coefficient for combined-unisono mode
    if (combinedUnison())
    {
        for (size_t i=1; i<unisonVoices(); ++i)
        {
            note += unisonNoteStep();
            freq = p_->noteFreq.frequency(note);

            // maximum (+/-) detune in Hertz
            const double maxdetune =
                    unisonDetune() / 200.0
                        // range of one note
                        * (p_->noteFreq.frequency(note + 1) - freq),

                    detune = (double)rand() / RAND_MAX
                             * maxdetune * 2. - maxdetune;

            voice->p_->freq = freq + detune;
            voice->p_->freq_c[i] = voice->p_->freq / sampleRate();
        }
        return voice;
    }

    const size_t numUnison = std::min(numberVoices(), unisonVoices());

    // generate inidivdual voices for each unisono voice
    SynthVoice * lastv = voice;
    for (size_t i=1; i<numUnison; ++i)
    {
        note += unisonNoteStep();
        freq = p_->noteFreq.frequency(note);

        // maximum (+/-) detune in Hertz
        const double maxdetune =
                unisonDetune() / 200.0
                    // range of one note
                    * (p_->noteFreq.frequency(note + 1) - freq),

                detune = (double)rand() / RAND_MAX
                             * maxdetune * 2. - maxdetune;

        SynthVoice * v = p_->noteOn(startSample, freq + detune,
                                    note, velocity, 1, userData);
        if (v)
        {
            lastv->p_->nextUnison = v;
            lastv = v;
        }
        else
        {
            SONOT_DEBUG_SYNTH("Synth::noteOn(): "
                              "failed to created all unisono voices "
                              << i << "/" << unisonVoices());
            return voice;
        }
    }

    return voice;
}

void Synth::noteOff(int note, size_t stopSample)
{
    p_->noteOff(stopSample, note);
}

void Synth::panic() { p_->panic(); }

void Synth::process(float *output, size_t bufferLength)
{
    p_->process(output, bufferLength);
}

void Synth::process(float ** output, size_t bufferLength)
{
    p_->process(output, bufferLength);
}


} // namespace Sonot