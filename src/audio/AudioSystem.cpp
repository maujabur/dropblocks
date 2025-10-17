#include "audio/AudioSystem.hpp"

#include <vector>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct AudioSystem::Impl {
    struct AudioDevice {
        SDL_AudioDeviceID device = 0;
        SDL_AudioSpec spec{};
        bool initialize() {
            spec.freq = 44100; spec.format = AUDIO_F32SYS; spec.channels = 1; spec.samples = 1024;
            device = SDL_OpenAudioDevice(nullptr, 0, &spec, &spec, 0);
            if (device) SDL_PauseAudioDevice(device, 0);
            return device != 0;
        }
        void cleanup() { if (device) SDL_CloseAudioDevice(device); device = 0; }
        void playBeep(double freq, int ms, float vol, bool square) {
            if (!device) return; int N = (int)(spec.freq * (ms / 1000.0)); std::vector<float> buf(N);
            double ph = 0, st = 2.0 * M_PI * freq / spec.freq; for (int i=0;i<N;i++){ float s = square ? (std::sin(ph)>=0?1.f:-1.f) : (float)std::sin(ph); buf[i]=s*vol; ph+=st; if (ph>2*M_PI) ph-=2*M_PI; }
            SDL_QueueAudio(device, buf.data(), (Uint32)(buf.size()*sizeof(float)));
        }
        void playChord(double baseFreq, int notes[], int count, int ms, float vol) {
            if (!device) return; for (int i=0;i<count;i++) playBeep(baseFreq*notes[i], ms, vol, false);
        }
        void playArpeggio(double baseFreq, int notes[], int count, int noteMs, float vol) {
            if (!device) return; for (int i=0;i<count;i++) playBeep(baseFreq*notes[i], noteMs, vol, false);
        }
        void playSweep(double startFreq, double endFreq, int ms, float vol) {
            if (!device) return; int steps=20; for(int i=0;i<=steps;i++){ double f=startFreq+(endFreq-startFreq)*(i/(double)steps); playBeep(f, ms/steps, vol, false);} }
    };

    AudioDevice device;
    AudioConfig config;
    Uint32 lastSweepSound = 0, lastScanlineSound = 0, lastMelody = 0, lastTension = 0;
};

AudioSystem::AudioSystem() : impl_(new Impl()) {}

bool AudioSystem::initialize() { return impl_->device.initialize(); }
void AudioSystem::cleanup() { impl_->device.cleanup(); }

// Synthesis
void AudioSystem::playBeep(double freq, int ms, float vol, bool square) { impl_->device.playBeep(freq, ms, vol * getConfig().masterVolume, square); }
void AudioSystem::playChord(double baseFreq, int notes[], int count, int ms, float vol) { impl_->device.playChord(baseFreq, notes, count, ms, vol * getConfig().masterVolume * getConfig().sfxVolume); }
void AudioSystem::playRotationSound(bool clockwise) { if (getConfig().enableMovementSounds) impl_->device.playBeep(clockwise?350.0:300.0, 15, 0.10f*getConfig().masterVolume, false); }
void AudioSystem::playMovementSound() { if (getConfig().enableMovementSounds) impl_->device.playBeep(150.0, 8, 0.06f*getConfig().masterVolume, true); }
void AudioSystem::playSoftDropSound() { if (getConfig().enableMovementSounds) impl_->device.playBeep(200.0, 12, 0.08f*getConfig().masterVolume, true); }
void AudioSystem::playHardDropSound() { if (getConfig().enableMovementSounds) impl_->device.playBeep(400.0, 20, 0.12f*getConfig().masterVolume, true); }
void AudioSystem::playKickSound() { if (getConfig().enableMovementSounds) impl_->device.playBeep(250.0, 15, 0.08f*getConfig().masterVolume, true); }
void AudioSystem::playLevelUpSound() { if (getConfig().enableLevelUpSounds) { float v=0.25f*getConfig().masterVolume; impl_->device.playBeep(880.0,100,v,false); impl_->device.playBeep(1320.0,80,v*0.8f,false);} }
void AudioSystem::playGameOverSound() { if (getConfig().enableLevelUpSounds){ float v=0.3f*getConfig().masterVolume; impl_->device.playBeep(440.0,200,v,false); impl_->device.playBeep(392.0,200,v,false); impl_->device.playBeep(349.0,200,v,false); impl_->device.playBeep(294.0,300,v*1.33f,false);} }
void AudioSystem::playComboSound(int combo) { if (getConfig().enableComboSounds && combo>1){ double f=440.0+(combo*50.0); float v=(0.15f+combo*0.02f)*getConfig().masterVolume*getConfig().sfxVolume; impl_->device.playBeep(f, 100+combo*20, v, true);} }
void AudioSystem::playTetrisSound() { if (getConfig().enableComboSounds){ int notes[]={1,5,8,12}; float v=0.20f*getConfig().masterVolume*getConfig().sfxVolume; impl_->device.playArpeggio(220.0, notes, 4, 50, v);} }
void AudioSystem::playBackgroundMelody(int level) { if (!getConfig().enableAmbientSounds) return; Uint32 now=SDL_GetTicks(); if (now-impl_->lastMelody>3000){ double base=220.0+(level*20.0); double melody[]={1.0,1.25,1.5,1.875,2.0}; for(int i=0;i<3;i++){ double f=base*melody[i%5]; float v=0.05f*getConfig().ambientVolume*getConfig().masterVolume; impl_->device.playBeep(f,200,v,false);} impl_->lastMelody=now; } }
void AudioSystem::playTensionSound(int filledRows) { if (!getConfig().enableAmbientSounds || filledRows<5) return; Uint32 now=SDL_GetTicks(); if (now-impl_->lastTension>1000){ float v=0.08f*getConfig().ambientVolume*getConfig().masterVolume; impl_->device.playBeep(80.0,300,v,true); impl_->lastTension=now; } }
void AudioSystem::playSweepEffect() { if (!getConfig().enableAmbientSounds) return; Uint32 now=SDL_GetTicks(); if (now-impl_->lastSweepSound>2000){ float v=0.03f*getConfig().ambientVolume*getConfig().masterVolume; impl_->device.playBeep(50.0,100,v,false); impl_->lastSweepSound=now; } }
void AudioSystem::playScanlineEffect() { if (!getConfig().enableAmbientSounds) return; Uint32 now=SDL_GetTicks(); if (now-impl_->lastScanlineSound>5000){ float v=0.02f*getConfig().ambientVolume*getConfig().masterVolume; impl_->device.playBeep(15.0,200,v,true); impl_->lastScanlineSound=now; } }
bool AudioSystem::loadFromConfig(const std::string& key, const std::string& value) { return getConfig().loadFromConfig(key, value); }

AudioConfig& AudioSystem::getConfig() { return impl_->config; }
const AudioConfig& AudioSystem::getConfig() const { return impl_->config; }


