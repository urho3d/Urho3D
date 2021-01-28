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

#pragma once

#include "../Audio/AudioDefs.h"
#include "../Scene/Component.h"

namespace Urho3D
{

class Audio;
class Sound;
class SoundStream;

/// Compressed audio decode buffer length in milliseconds.
static const int STREAM_BUFFER_LENGTH = 100;

/// %Sound source component with stereo position. A sound source needs to be created to a node to be considered "enabled" and be able to play, however that node does not need to belong to a scene.
class URHO3D_API SoundSource : public Component
{
    URHO3D_OBJECT(SoundSource, Component);

public:
    /// Construct.
    explicit SoundSource(Context* context);
    /// Destruct. Remove self from the audio subsystem.
    ~SoundSource() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Seek to time.
    void Seek(float seekTime);
    /// Play a sound.
    void Play(Sound* sound);
    /// Play a sound with specified frequency.
    void Play(Sound* sound, float frequency);
    /// Play a sound with specified frequency and gain.
    void Play(Sound* sound, float frequency, float gain);
    /// Play a sound with specified frequency, gain and panning.
    void Play(Sound* sound, float frequency, float gain, float panning);
    /// Start playing a sound stream.
    void Play(SoundStream* stream);
    /// Stop playback.
    void Stop();
    /// Set sound type, determines the master gain group.
    /// @property
    void SetSoundType(const String& type);
    /// Set frequency.
    /// @property
    void SetFrequency(float frequency);
    /// Set gain. 0.0 is silence, 1.0 is full volume.
    /// @property
    void SetGain(float gain);
    /// Set attenuation. 1.0 is unaltered. Used for distance attenuated playback.
    void SetAttenuation(float attenuation);
    /// Set stereo panning. -1.0 is full left and 1.0 is full right.
    /// @property
    void SetPanning(float panning);
    /// Set to remove either the sound source component or its owner node from the scene automatically on sound playback completion. Disabled by default.
    /// @property
    void SetAutoRemoveMode(AutoRemoveMode mode);
    /// Set new playback position.
    void SetPlayPosition(signed char* pos);

    /// Return sound.
    /// @property
    Sound* GetSound() const { return sound_; }

    /// Return playback position.
    volatile signed char* GetPlayPosition() const { return position_; }

    /// Return sound type, determines the master gain group.
    /// @property
    String GetSoundType() const { return soundType_; }

    /// Return playback time position.
    /// @property
    float GetTimePosition() const { return timePosition_; }

    /// Return frequency.
    /// @property
    float GetFrequency() const { return frequency_; }

    /// Return gain.
    /// @property
    float GetGain() const { return gain_; }

    /// Return attenuation.
    /// @property
    float GetAttenuation() const { return attenuation_; }

    /// Return stereo panning.
    /// @property
    float GetPanning() const { return panning_; }

    /// Return automatic removal mode on sound playback completion.
    /// @property
    AutoRemoveMode GetAutoRemoveMode() const { return autoRemove_; }

    /// Return whether is playing.
    /// @property
    bool IsPlaying() const;

    /// Update the sound source. Perform subclass specific operations. Called by Audio.
    virtual void Update(float timeStep);
    /// Mix sound source output to a 32-bit clipping buffer. Called by Audio.
    void Mix(int* dest, unsigned samples, int mixRate, bool stereo, bool interpolation);
    /// Update the effective master gain. Called internally and by Audio when the master gain changes.
    void UpdateMasterGain();

    /// Set sound attribute.
    void SetSoundAttr(const ResourceRef& value);
    /// Set sound position attribute.
    void SetPositionAttr(int value);
    /// Return sound attribute.
    ResourceRef GetSoundAttr() const;
    /// Set sound playing attribute.
    void SetPlayingAttr(bool value);
    /// Return sound position attribute.
    int GetPositionAttr() const;

protected:
    /// Audio subsystem.
    WeakPtr<Audio> audio_;
    /// SoundSource type, determines the master gain group.
    String soundType_;
    /// SoundSource type hash.
    StringHash soundTypeHash_;
    /// Frequency.
    float frequency_;
    /// Gain.
    float gain_;
    /// Attenuation.
    float attenuation_;
    /// Stereo panning.
    float panning_;
    /// Effective master gain.
    float masterGain_{};
    /// Whether finished event should be sent on playback stop.
    bool sendFinishedEvent_;
    /// Automatic removal mode.
    AutoRemoveMode autoRemove_;

private:
    /// Play a sound without locking the audio mutex. Called internally.
    void PlayLockless(Sound* sound);
    /// Play a sound stream without locking the audio mutex. Called internally.
    void PlayLockless(const SharedPtr<SoundStream>& stream);
    /// Stop sound without locking the audio mutex. Called internally.
    void StopLockless();
    /// Set new playback position without locking the audio mutex. Called internally.
    void SetPlayPositionLockless(signed char* pos);
    /// Mix mono sample to mono buffer.
    void MixMonoToMono(Sound* sound, int* dest, unsigned samples, int mixRate);
    /// Mix mono sample to stereo buffer.
    void MixMonoToStereo(Sound* sound, int* dest, unsigned samples, int mixRate);
    /// Mix mono sample to mono buffer interpolated.
    void MixMonoToMonoIP(Sound* sound, int* dest, unsigned samples, int mixRate);
    /// Mix mono sample to stereo buffer interpolated.
    void MixMonoToStereoIP(Sound* sound, int* dest, unsigned samples, int mixRate);
    /// Mix stereo sample to mono buffer.
    void MixStereoToMono(Sound* sound, int* dest, unsigned samples, int mixRate);
    /// Mix stereo sample to stereo buffer.
    void MixStereoToStereo(Sound* sound, int* dest, unsigned samples, int mixRate);
    /// Mix stereo sample to mono buffer interpolated.
    void MixStereoToMonoIP(Sound* sound, int* dest, unsigned samples, int mixRate);
    /// Mix stereo sample to stereo buffer interpolated.
    void MixStereoToStereoIP(Sound* sound, int* dest, unsigned samples, int mixRate);
    /// Advance playback pointer without producing audible output.
    void MixZeroVolume(Sound* sound, unsigned samples, int mixRate);
    /// Advance playback pointer to simulate audio playback in headless mode.
    void MixNull(float timeStep);

    /// Sound that is being played.
    SharedPtr<Sound> sound_;
    /// Sound stream that is being played.
    SharedPtr<SoundStream> soundStream_;
    /// Playback position.
    volatile signed char* position_;
    /// Playback fractional position.
    volatile int fractPosition_;
    /// Playback time position.
    volatile float timePosition_;
    /// Decode buffer.
    SharedPtr<Sound> streamBuffer_;
    /// Unused stream bytes from previous frame.
    int unusedStreamSize_;

    /// Current volume state of left channel. Value -1 means not assigned.
    int currentVolumeL_{-1};
    /// Current volume state of right channel. Value -1 means not assigned (redundant).
    int currentVolumeR_{-1};
};

}
