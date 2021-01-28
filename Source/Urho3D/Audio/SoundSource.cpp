//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Audio/Audio.h"
#include "../Audio/AudioEvents.h"
#include "../Audio/Sound.h"
#include "../Audio/SoundSource.h"
#include "../Audio/SoundStream.h"
#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Node.h"
#include "../Scene/ReplicationState.h"

#include "../DebugNew.h"

// testing option
//#define DISABLE_SMOOTH_VOLUME

namespace Urho3D
{

#define INC_POS_LOOPED() \
    pos += intAdd; \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        ++pos; \
    } \
    while (pos >= end) \
        pos -= (end - repeat); \

#define INC_POS_ONESHOT() \
    pos += intAdd; \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        ++pos; \
    } \
    if (pos >= end) \
    { \
        pos = 0; \
        break; \
    } \

#define INC_POS_STEREO_LOOPED() \
    pos += ((unsigned)intAdd << 1u); \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        pos += 2; \
    } \
    while (pos >= end) \
        pos -= (end - repeat); \

#define INC_POS_STEREO_ONESHOT() \
    pos += ((unsigned)intAdd << 1u); \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        pos += 2; \
    } \
    if (pos >= end) \
    { \
        pos = 0; \
        break; \
    } \

#define GET_IP_SAMPLE() (((((int)pos[1] - (int)pos[0]) * fractPos) / 65536) + (int)pos[0])

#define GET_IP_SAMPLE_LEFT() (((((int)pos[2] - (int)pos[0]) * fractPos) / 65536) + (int)pos[0])

#define GET_IP_SAMPLE_RIGHT() (((((int)pos[3] - (int)pos[1]) * fractPos) / 65536) + (int)pos[1])

static const int STREAM_SAFETY_SAMPLES = 4;

// Valid values : 2^n, n = 8 .. 15.
static const int VOLUME_DENOM = 4096;

extern const char* AUDIO_CATEGORY;

extern const char* autoRemoveModeNames[];

SoundSource::SoundSource(Context* context) :
    Component(context),
    soundType_(SOUND_EFFECT),
    frequency_(0.0f),
    gain_(1.0f),
    attenuation_(1.0f),
    panning_(0.0f),
    sendFinishedEvent_(false),
    autoRemove_(REMOVE_DISABLED),
    position_(nullptr),
    fractPosition_(0),
    timePosition_(0.0f),
    unusedStreamSize_(0)
{
    audio_ = GetSubsystem<Audio>();

    if (audio_)
        audio_->AddSoundSource(this);

    UpdateMasterGain();
}

SoundSource::~SoundSource()
{
    if (audio_)
        audio_->RemoveSoundSource(this);
}

void SoundSource::RegisterObject(Context* context)
{
    context->RegisterFactory<SoundSource>(AUDIO_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Sound", GetSoundAttr, SetSoundAttr, ResourceRef, ResourceRef(Sound::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Type", GetSoundType, SetSoundType, String, SOUND_EFFECT, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Frequency", float, frequency_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Gain", float, gain_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Attenuation", float, attenuation_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Panning", float, panning_, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Playing", IsPlaying, SetPlayingAttr, bool, false, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE("Autoremove Mode", autoRemove_, autoRemoveModeNames, REMOVE_DISABLED, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Play Position", GetPositionAttr, SetPositionAttr, int, 0, AM_FILE);
}

void SoundSource::Seek(float seekTime)
{
    // Ignore buffered sound stream
    if (!audio_ || !sound_ || (soundStream_ && !sound_->IsCompressed()))
        return;

    // Set to valid range
    seekTime = Clamp(seekTime, 0.0f, sound_->GetLength());

    if (!soundStream_)
    {
        // Raw or wav format
        SetPositionAttr((int)(seekTime * (sound_->GetSampleSize() * sound_->GetFrequency())));
    }
    else
    {
        // Ogg format
        if (soundStream_->Seek((unsigned)(seekTime * soundStream_->GetFrequency())))
        {
            timePosition_ = seekTime;
        }
    }
}

void SoundSource::Play(Sound* sound)
{
    if (!audio_)
        return;

    // If no frequency set yet, set from the sound's default
    if (frequency_ == 0.0f && sound)
        SetFrequency(sound->GetFrequency());

    // If sound source is currently playing, have to lock the audio mutex
    if (position_)
    {
        MutexLock lock(audio_->GetMutex());
        currentVolumeL_ = currentVolumeR_ = -1;
        PlayLockless(sound);
    }
    else
    {
        currentVolumeL_ = currentVolumeR_ = -1;
        PlayLockless(sound);
    }

    // Forget the Sound & Is Playing attribute previous values so that they will be sent again, triggering
    // the sound correctly on network clients even after the initial playback
    if (networkState_ && networkState_->attributes_ && networkState_->previousValues_.Size())
    {
        for (unsigned i = 1; i < networkState_->previousValues_.Size(); ++i)
        {
            // The indexing is different for SoundSource & SoundSource3D, as SoundSource3D removes two attributes,
            // so go by attribute types
            VariantType type = networkState_->attributes_->At(i).type_;
            if (type == VAR_RESOURCEREF || type == VAR_BOOL)
                networkState_->previousValues_[i] = Variant::EMPTY;
        }
    }

    MarkNetworkUpdate();
}

void SoundSource::Play(Sound* sound, float frequency)
{
    SetFrequency(frequency);
    Play(sound);
}

void SoundSource::Play(Sound* sound, float frequency, float gain)
{
    SetFrequency(frequency);
    SetGain(gain);
    Play(sound);
}

void SoundSource::Play(Sound* sound, float frequency, float gain, float panning)
{
    SetFrequency(frequency);
    SetGain(gain);
    SetPanning(panning);
    Play(sound);
}

void SoundSource::Play(SoundStream* stream)
{
    if (!audio_)
        return;

    // If no frequency set yet, set from the stream's default
    if (frequency_ == 0.0f && stream)
        SetFrequency(stream->GetFrequency());

    SharedPtr<SoundStream> streamPtr(stream);

    // If sound source is currently playing, have to lock the audio mutex. When stream playback is explicitly
    // requested, clear the existing sound if any
    if (position_)
    {
        MutexLock lock(audio_->GetMutex());
        currentVolumeL_ = currentVolumeR_ = -1;
        sound_.Reset();
        PlayLockless(streamPtr);
    }
    else
    {
        currentVolumeL_ = currentVolumeR_ = -1;
        sound_.Reset();
        PlayLockless(streamPtr);
    }

    // Stream playback is not supported for network replication, no need to mark network dirty
}

void SoundSource::Stop()
{
    if (!audio_)
        return;

    // If sound source is currently playing, have to lock the audio mutex
    if (position_)
    {
        MutexLock lock(audio_->GetMutex());
        StopLockless();
    }
    else
        StopLockless();

    MarkNetworkUpdate();
}

void SoundSource::SetSoundType(const String& type)
{
    if (type == SOUND_MASTER)
        return;

    soundType_ = type;
    soundTypeHash_ = StringHash(type);
    UpdateMasterGain();

    MarkNetworkUpdate();
}

void SoundSource::SetFrequency(float frequency)
{
    frequency_ = Clamp(frequency, 0.0f, 535232.0f);
    MarkNetworkUpdate();
}

void SoundSource::SetGain(float gain)
{
    gain_ = Max(gain, 0.0f);
    MarkNetworkUpdate();
}

void SoundSource::SetAttenuation(float attenuation)
{
    attenuation_ = Clamp(attenuation, 0.0f, 1.0f);
    MarkNetworkUpdate();
}

void SoundSource::SetPanning(float panning)
{
    panning_ = Clamp(panning, -1.0f, 1.0f);
    MarkNetworkUpdate();
}

void SoundSource::SetAutoRemoveMode(AutoRemoveMode mode)
{
    autoRemove_ = mode;
    MarkNetworkUpdate();
}

bool SoundSource::IsPlaying() const
{
    return (sound_ || soundStream_) && position_ != nullptr;
}

void SoundSource::SetPlayPosition(signed char* pos)
{
    // Setting play position on a stream is not supported
    if (!audio_ || !sound_ || soundStream_)
        return;

    MutexLock lock(audio_->GetMutex());
    SetPlayPositionLockless(pos);
}

void SoundSource::Update(float timeStep)
{
    if (!audio_ || !IsEnabledEffective())
        return;

    // If there is no actual audio output, perform fake mixing into a nonexistent buffer to check stopping/looping
    if (!audio_->IsInitialized())
        MixNull(timeStep);

    // Free the stream if playback has stopped
    if (soundStream_ && !position_)
        StopLockless();

    bool playing = IsPlaying();

    if (!playing && sendFinishedEvent_)
    {
        sendFinishedEvent_ = false;

        // Make a weak pointer to self to check for destruction during event handling
        WeakPtr<SoundSource> self(this);

        using namespace SoundFinished;

        VariantMap& eventData = context_->GetEventDataMap();
        eventData[P_NODE] = node_;
        eventData[P_SOUNDSOURCE] = this;
        eventData[P_SOUND] = sound_;
        node_->SendEvent(E_SOUNDFINISHED, eventData);

        if (self.Expired())
            return;

        DoAutoRemove(autoRemove_);
    }
}

void SoundSource::Mix(int* dest, unsigned samples, int mixRate, bool stereo, bool interpolation)
{
    if (!position_ || (!sound_ && !soundStream_) || !IsEnabledEffective())
        return;

    int streamFilledSize, outBytes;

    if (soundStream_ && streamBuffer_)
    {
        int streamBufferSize = streamBuffer_->GetDataSize();
        // Calculate how many bytes of stream sound data is needed
        auto neededSize = (int)((float)samples * frequency_ / (float)mixRate);
        // Add a little safety buffer. Subtract previous unused data
        neededSize += STREAM_SAFETY_SAMPLES;
        neededSize *= soundStream_->GetSampleSize();
        neededSize -= unusedStreamSize_;
        neededSize = Clamp(neededSize, 0, streamBufferSize - unusedStreamSize_);

        // Always start play position at the beginning of the stream buffer
        position_ = streamBuffer_->GetStart();

        // Request new data from the stream
        signed char* destination = streamBuffer_->GetStart() + unusedStreamSize_;
        outBytes = neededSize ? soundStream_->GetData(destination, (unsigned)neededSize) : 0;
        destination += outBytes;
        // Zero-fill rest if stream did not produce enough data
        if (outBytes < neededSize)
            memset(destination, 0, (size_t)(neededSize - outBytes));

        // Calculate amount of total bytes of data in stream buffer now, to know how much went unused after mixing
        streamFilledSize = neededSize + unusedStreamSize_;
    }

    // If streaming, play the stream buffer. Otherwise play the original sound
    Sound* sound = soundStream_ ? streamBuffer_ : sound_;
    if (!sound)
        return;

    // Choose the correct mixing routine
    if (!sound->IsStereo())
    {
        if (interpolation)
        {
            if (stereo)
                MixMonoToStereoIP(sound, dest, samples, mixRate);
            else
                MixMonoToMonoIP(sound, dest, samples, mixRate);
        }
        else
        {
            if (stereo)
                MixMonoToStereo(sound, dest, samples, mixRate);
            else
                MixMonoToMono(sound, dest, samples, mixRate);
        }
    }
    else
    {
        if (interpolation)
        {
            if (stereo)
                MixStereoToStereoIP(sound, dest, samples, mixRate);
            else
                MixStereoToMonoIP(sound, dest, samples, mixRate);
        }
        else
        {
            if (stereo)
                MixStereoToStereo(sound, dest, samples, mixRate);
            else
                MixStereoToMono(sound, dest, samples, mixRate);
        }
    }

    // Update the time position. In stream mode, copy unused data back to the beginning of the stream buffer
    if (soundStream_)
    {
        timePosition_ += ((float)samples / (float)mixRate) * frequency_ / soundStream_->GetFrequency();

        unusedStreamSize_ = Max(streamFilledSize - (int)(size_t)(position_ - streamBuffer_->GetStart()), 0);
        if (unusedStreamSize_)
            memcpy(streamBuffer_->GetStart(), (const void*)position_, (size_t)unusedStreamSize_);

        // If stream did not produce any data, stop if applicable
        if (!outBytes && soundStream_->GetStopAtEnd())
        {
            position_ = nullptr;
            return;
        }
    }
    else if (sound_)
        timePosition_ = ((float)(int)(size_t)(position_ - sound_->GetStart())) / (sound_->GetSampleSize() * sound_->GetFrequency());
}

void SoundSource::UpdateMasterGain()
{
    if (audio_)
        masterGain_ = audio_->GetSoundSourceMasterGain(soundType_);
}

void SoundSource::SetSoundAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* newSound = cache->GetResource<Sound>(value.name_);
    if (IsPlaying())
        Play(newSound);
    else
    {
        // When changing the sound and not playing, free previous sound stream and stream buffer (if any)
        soundStream_.Reset();
        streamBuffer_.Reset();
        sound_ = newSound;
    }
}

void SoundSource::SetPlayingAttr(bool value)
{
    if (value)
    {
        if (!IsPlaying())
            Play(sound_);
    }
    else
        Stop();
}

void SoundSource::SetPositionAttr(int value)
{
    if (sound_)
        SetPlayPosition(sound_->GetStart() + value);
}

ResourceRef SoundSource::GetSoundAttr() const
{
    return GetResourceRef(sound_, Sound::GetTypeStatic());
}

int SoundSource::GetPositionAttr() const
{
    if (sound_ && position_)
        return (int)(GetPlayPosition() - sound_->GetStart());
    else
        return 0;
}

void SoundSource::PlayLockless(Sound* sound)
{
    // Reset the time position in any case
    timePosition_ = 0.0f;

    if (sound)
    {
        if (!sound->IsCompressed())
        {
            // Uncompressed sound start
            signed char* start = sound->GetStart();
            if (start)
            {
                // Free existing stream & stream buffer if any
                soundStream_.Reset();
                streamBuffer_.Reset();
                sound_ = sound;
                position_ = start;
                fractPosition_ = 0;
                sendFinishedEvent_ = true;
                return;
            }
        }
        else
        {
            // Compressed sound start
            PlayLockless(sound->GetDecoderStream());
            sound_ = sound;
            return;
        }
    }

    // If sound pointer is null or if sound has no data, stop playback
    StopLockless();
    sound_.Reset();
}

void SoundSource::PlayLockless(const SharedPtr<SoundStream>& stream)
{
    // Reset the time position in any case
    timePosition_ = 0.0f;

    if (stream)
    {
        // Setup the stream buffer
        unsigned sampleSize = stream->GetSampleSize();
        unsigned streamBufferSize = sampleSize * stream->GetIntFrequency() * STREAM_BUFFER_LENGTH / 1000;

        streamBuffer_ = new Sound(context_);
        streamBuffer_->SetSize(streamBufferSize);
        streamBuffer_->SetFormat(stream->GetIntFrequency(), stream->IsSixteenBit(), stream->IsStereo());
        streamBuffer_->SetLooped(true);

        soundStream_ = stream;
        unusedStreamSize_ = 0;
        position_ = streamBuffer_->GetStart();
        fractPosition_ = 0;
        sendFinishedEvent_ = true;
        return;
    }

    // If stream pointer is null, stop playback
    StopLockless();
}

void SoundSource::StopLockless()
{
    position_ = nullptr;
    timePosition_ = 0.0f;

    // Free the sound stream and decode buffer if a stream was playing
    soundStream_.Reset();
    streamBuffer_.Reset();
}

void SoundSource::SetPlayPositionLockless(signed char* pos)
{
    // Setting position on a stream is not supported
    if (!sound_ || soundStream_)
        return;

    signed char* start = sound_->GetStart();
    signed char* end = sound_->GetEnd();
    if (pos < start)
        pos = start;
    if (sound_->IsSixteenBit() && (pos - start) & 1u)
        ++pos;
    if (pos > end)
        pos = end;

    position_ = pos;
    timePosition_ = ((float)(int)(size_t)(pos - sound_->GetStart())) / (sound_->GetSampleSize() * sound_->GetFrequency());
}

void SoundSource::MixMonoToMono(Sound* sound, int* dest, unsigned samples, int mixRate)
{
    int denom = sound->IsSixteenBit() ? VOLUME_DENOM : (VOLUME_DENOM >> 8);

    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt((float)VOLUME_DENOM * totalGain);

    int& currentVolume = currentVolumeL_;
    if (currentVolume == -1) currentVolume = vol;
    if (!vol && !currentVolume)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    int volDelta = Clamp(vol - currentVolume, -1, 1);
    unsigned rampSamples = Min(samples, (unsigned)((vol - currentVolume) * volDelta));
#ifdef DISABLE_SMOOTH_VOLUME
    rampSamples = 0;
#endif
    samples -= rampSamples;
    float add = frequency_ / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (*pos * currentVolume) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (*pos * vol) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (*pos * currentVolume) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (*pos * vol) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (*pos * currentVolume) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (*pos * vol) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (*pos * currentVolume) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (*pos * vol) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }
    fractPosition_ = fractPos;
}

void SoundSource::MixMonoToStereo(Sound* sound, int* dest, unsigned samples, int mixRate)
{
    int denom = sound->IsSixteenBit() ? VOLUME_DENOM : (VOLUME_DENOM >> 8);

    float totalGain = masterGain_ * attenuation_ * gain_;
    auto leftVol = RoundToInt((-panning_ + 1.0f) * (float)VOLUME_DENOM * totalGain);
    auto rightVol = RoundToInt((panning_ + 1.0f) * (float)VOLUME_DENOM * totalGain);

    if (currentVolumeL_ == -1)
    {
        currentVolumeL_ = leftVol;
        currentVolumeR_ = rightVol;
    }
    if (!currentVolumeL_ && !currentVolumeR_ && !leftVol && !rightVol)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    int leftVolDelta = Clamp(leftVol - currentVolumeL_, -1, 1);
    int rightVolDelta = Clamp(rightVol - currentVolumeR_, -1, 1);

    unsigned leftRampSamples = Min(samples, (unsigned)((leftVol - currentVolumeL_) * leftVolDelta));
    unsigned rightRampSamples = Min(samples, (unsigned)((rightVol - currentVolumeR_) * rightVolDelta));
    unsigned rampSamples = Max(leftRampSamples, rightRampSamples);
#ifdef DISABLE_SMOOTH_VOLUME
    rampSamples = 0;
#endif
    samples -= rampSamples;
    float add = frequency_ / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolumeL_ += leftVolDelta * (currentVolumeL_ != leftVol);
                currentVolumeR_ += rightVolDelta * (currentVolumeR_ != rightVol);
                *dest = *dest + (*pos * currentVolumeL_) / denom;
                ++dest;
                *dest = *dest + (*pos * currentVolumeR_) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (*pos * leftVol) / denom;
                ++dest;
                *dest = *dest + (*pos * rightVol) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolumeL_ += leftVolDelta * (currentVolumeL_ != leftVol);
                currentVolumeR_ += rightVolDelta * (currentVolumeR_ != rightVol);
                *dest = *dest + (*pos * currentVolumeL_) / denom;
                ++dest;
                *dest = *dest + (*pos * currentVolumeR_) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (*pos * leftVol) / denom;
                ++dest;
                *dest = *dest + (*pos * rightVol) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolumeL_ += leftVolDelta * (currentVolumeL_ != leftVol);
                currentVolumeR_ += rightVolDelta * (currentVolumeR_ != rightVol);
                *dest = *dest + (*pos * currentVolumeL_) / denom;
                ++dest;
                *dest = *dest + (*pos * currentVolumeR_) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (*pos * leftVol) / denom;
                ++dest;
                *dest = *dest + (*pos * rightVol) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolumeL_ += leftVolDelta * (currentVolumeL_ != leftVol);
                currentVolumeR_ += rightVolDelta * (currentVolumeR_ != rightVol);
                *dest = *dest + (*pos * currentVolumeL_) / denom;
                ++dest;
                *dest = *dest + (*pos * currentVolumeR_) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (*pos * leftVol) / denom;
                ++dest;
                *dest = *dest + (*pos * rightVol) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixMonoToMonoIP(Sound* sound, int* dest, unsigned samples, int mixRate)
{
    int denom = sound->IsSixteenBit() ? VOLUME_DENOM : (VOLUME_DENOM >> 8);

    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt((float)VOLUME_DENOM * totalGain);

    int& currentVolume = currentVolumeL_;
    if (currentVolume == -1) currentVolume = vol;
    if (!vol && !currentVolume)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    int volDelta = Clamp(vol - currentVolume, -1, 1);
    unsigned rampSamples = Min(samples, (unsigned)((vol - currentVolume) * volDelta));
#ifdef DISABLE_SMOOTH_VOLUME
    rampSamples = 0;
#endif
    samples -= rampSamples;
    float add = frequency_ / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (GET_IP_SAMPLE() * currentVolume) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE() * vol) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (GET_IP_SAMPLE() * currentVolume) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE() * vol) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (GET_IP_SAMPLE() * currentVolume) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE() * vol) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (GET_IP_SAMPLE() * currentVolume) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE() * vol) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixMonoToStereoIP(Sound* sound, int* dest, unsigned samples, int mixRate)
{
    int denom = sound->IsSixteenBit() ? VOLUME_DENOM : (VOLUME_DENOM >> 8);

    float totalGain = masterGain_ * attenuation_ * gain_;
    auto leftVol = RoundToInt((-panning_ + 1.0f) * (float)VOLUME_DENOM * totalGain);
    auto rightVol = RoundToInt((panning_ + 1.0f) * (float)VOLUME_DENOM * totalGain);

    if (currentVolumeL_ == -1)
    {
        currentVolumeL_ = leftVol;
        currentVolumeR_ = rightVol;
    }
    if(!currentVolumeL_ && !currentVolumeR_ && !leftVol && !rightVol)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    int leftVolDelta = Clamp(leftVol  - currentVolumeL_, -1, 1);
    int rightVolDelta = Clamp(rightVol - currentVolumeR_, -1, 1);

    unsigned leftRampSamples = Min(samples, (unsigned)((leftVol - currentVolumeL_) * leftVolDelta));
    unsigned rightRampSamples = Min(samples, (unsigned)((rightVol - currentVolumeR_) * rightVolDelta));
    unsigned rampSamples = Max(leftRampSamples, rightRampSamples);
#ifdef DISABLE_SMOOTH_VOLUME
    rampSamples = 0;
#endif
    samples -= rampSamples;
    float add = frequency_ / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolumeL_ += leftVolDelta * (currentVolumeL_ != leftVol);
                currentVolumeR_ += rightVolDelta * (currentVolumeR_ != rightVol);
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * currentVolumeL_) / denom;
                ++dest;
                *dest = *dest + (s * currentVolumeR_) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * leftVol) / denom;
                ++dest;
                *dest = *dest + (s * rightVol) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolumeL_ += leftVolDelta * (currentVolumeL_ != leftVol);
                currentVolumeR_ += rightVolDelta * (currentVolumeR_ != rightVol);
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * currentVolumeL_) / denom;
                ++dest;
                *dest = *dest + (s * currentVolumeR_) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * leftVol) / denom;
                ++dest;
                *dest = *dest + (s * rightVol) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolumeL_ += leftVolDelta * (currentVolumeL_ != leftVol);
                currentVolumeR_ += rightVolDelta * (currentVolumeR_ != rightVol);
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * currentVolumeL_) / denom;
                ++dest;
                *dest = *dest + (s * currentVolumeR_) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * leftVol) / denom;
                ++dest;
                *dest = *dest + (s * rightVol) / denom;
                ++dest;
                INC_POS_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolumeL_ += leftVolDelta * (currentVolumeL_ != leftVol);
                currentVolumeR_ += rightVolDelta * (currentVolumeR_ != rightVol);
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * currentVolumeL_) / denom;
                ++dest;
                *dest = *dest + (s * currentVolumeR_) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = GET_IP_SAMPLE();
                *dest = *dest + (s * leftVol) / denom;
                ++dest;
                *dest = *dest + (s * rightVol) / denom;
                ++dest;
                INC_POS_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixStereoToMono(Sound* sound, int* dest, unsigned samples, int mixRate)
{
    int denom = sound->IsSixteenBit() ? VOLUME_DENOM : (VOLUME_DENOM >> 8);

    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt((float)VOLUME_DENOM * totalGain);

    int& currentVolume = currentVolumeL_;
    if (currentVolume == -1) currentVolume = vol;
    if (!vol && !currentVolume)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    int volDelta = Clamp(vol - currentVolume, -1, 1);
    unsigned rampSamples = Min(samples, (unsigned)((vol - currentVolume) * volDelta));
#ifdef DISABLE_SMOOTH_VOLUME
    rampSamples = 0;
#endif
    samples -= rampSamples;
    float add = frequency_ / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + (s * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + (s * vol) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + (s * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + (s * vol) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + (s * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + (s * vol) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + (s * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = ((int)pos[0] + (int)pos[1]) / 2;
                *dest = *dest + (s * vol) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixStereoToStereo(Sound* sound, int* dest, unsigned samples, int mixRate)
{
    int denom = sound->IsSixteenBit() ? VOLUME_DENOM : (VOLUME_DENOM >> 8);

    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt((float)VOLUME_DENOM * totalGain);

    int& currentVolume = currentVolumeL_;
    if (currentVolume == -1) currentVolume = vol;
    if (!vol && !currentVolume)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    int volDelta = Clamp(vol - currentVolume, -1, 1);
    unsigned rampSamples = Min(samples, (unsigned)((vol - currentVolume) * volDelta));
#ifdef DISABLE_SMOOTH_VOLUME
    rampSamples = 0;
#endif
    samples -= rampSamples;
    float add = frequency_ / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (pos[0] * currentVolume) / denom;
                ++dest;
                *dest = *dest + (pos[1] * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (pos[0] * vol) / denom;
                ++dest;
                *dest = *dest + (pos[1] * vol) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (pos[0] * currentVolume) / denom;
                ++dest;
                *dest = *dest + (pos[1] * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (pos[0] * vol) / denom;
                ++dest;
                *dest = *dest + (pos[1] * vol) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (pos[0] * currentVolume) / denom;
                ++dest;
                *dest = *dest + (pos[1] * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (pos[0] * vol) / denom;
                ++dest;
                *dest = *dest + (pos[1] * vol) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (pos[0] * currentVolume) / denom;
                ++dest;
                *dest = *dest + (pos[1] * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (pos[0] * vol) / denom;
                ++dest;
                *dest = *dest + (pos[1] * vol) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixStereoToMonoIP(Sound* sound, int* dest, unsigned samples, int mixRate)
{
    int denom = sound->IsSixteenBit() ? VOLUME_DENOM : (VOLUME_DENOM >> 8);

    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt((float)VOLUME_DENOM * totalGain);
    int& currentVolume = currentVolumeL_;
    if (currentVolume == -1) currentVolume = vol;
    if (!vol && !currentVolume)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    int volDelta = Clamp(vol - currentVolume, -1, 1);
    unsigned rampSamples = Min(samples, (unsigned)((vol - currentVolume) * volDelta));
#ifdef DISABLE_SMOOTH_VOLUME
    rampSamples = 0;
#endif
    samples -= rampSamples;
    float add = frequency_ / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + (s * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + (s * vol) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + (s * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + (s * vol) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + (s * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + (s * vol) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + (s * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                int s = (GET_IP_SAMPLE_LEFT() + GET_IP_SAMPLE_RIGHT()) / 2;
                *dest = *dest + (s * vol) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixStereoToStereoIP(Sound* sound, int* dest, unsigned samples, int mixRate)
{
    int denom = sound->IsSixteenBit() ? VOLUME_DENOM : (VOLUME_DENOM >> 8);

    float totalGain = masterGain_ * attenuation_ * gain_;
    auto vol = RoundToInt((float)VOLUME_DENOM * totalGain);
    int& currentVolume = currentVolumeL_;
    if (currentVolume == -1) currentVolume = vol;
    if (!vol && !currentVolume)
    {
        MixZeroVolume(sound, samples, mixRate);
        return;
    }

    int volDelta = Clamp(vol - currentVolume, -1, 1);
    unsigned rampSamples = Min(samples, (unsigned)((vol - currentVolume) * volDelta));
#ifdef DISABLE_SMOOTH_VOLUME
    rampSamples = 0;
#endif
    samples -= rampSamples;
    float add = frequency_ / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    int fractPos = fractPosition_;

    if (sound->IsSixteenBit())
    {
        auto* pos = (short*)position_;
        auto* end = (short*)sound->GetEnd();
        auto* repeat = (short*)sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * currentVolume) / denom;
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * vol) / denom;
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * vol) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = (signed char*)pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * currentVolume) / denom;
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * vol) / denom;
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * vol) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = (signed char*)pos;
        }
    }
    else
    {
        auto* pos = (signed char*)position_;
        signed char* end = sound->GetEnd();
        signed char* repeat = sound->GetRepeat();

        if (sound->IsLooped())
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * currentVolume) / denom;
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * vol) / denom;
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * vol) / denom;
                ++dest;
                INC_POS_STEREO_LOOPED();
            }
            position_ = pos;
        }
        else
        {
            while (rampSamples--)
            {
                currentVolume += volDelta * (currentVolume != vol);
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * currentVolume) / denom;
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * currentVolume) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            if (!~rampSamples)
            while (samples--)
            {
                *dest = *dest + (GET_IP_SAMPLE_LEFT() * vol) / denom;
                ++dest;
                *dest = *dest + (GET_IP_SAMPLE_RIGHT() * vol) / denom;
                ++dest;
                INC_POS_STEREO_ONESHOT();
            }
            position_ = pos;
        }
    }

    fractPosition_ = fractPos;
}

void SoundSource::MixZeroVolume(Sound* sound, unsigned samples, int mixRate)
{
    float add = frequency_ * (float)samples / (float)mixRate;
    auto intAdd = (int)add;
    auto fractAdd = (int)((add - floorf(add)) * 65536.0f);
    unsigned sampleSize = sound->GetSampleSize();

    fractPosition_ += fractAdd;
    if (fractPosition_ > 65535)
    {
        fractPosition_ &= 65535;
        position_ += sampleSize;
    }
    position_ += intAdd * sampleSize;

    if (position_ > sound->GetEnd())
    {
        if (sound->IsLooped())
        {
            while (position_ >= sound->GetEnd())
            {
                position_ -= (sound->GetEnd() - sound->GetRepeat());
            }
        }
        else
            position_ = nullptr;
    }
}

void SoundSource::MixNull(float timeStep)
{
    if (!position_ || !sound_ || !IsEnabledEffective())
        return;

    // Advance only the time position
    timePosition_ += timeStep * frequency_ / sound_->GetFrequency();

    if (sound_->IsLooped())
    {
        // For simulated playback, simply reset the time position to zero when the sound loops
        if (timePosition_ >= sound_->GetLength())
            timePosition_ -= sound_->GetLength();
    }
    else
    {
        if (timePosition_ >= sound_->GetLength())
        {
            position_ = nullptr;
            timePosition_ = 0.0f;
        }
    }
}

}
