#include "audio/Audio.h"

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_FLAC
#define MA_NO_MP3
#include <miniaudio.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace audio {
namespace {

constexpr int   VOICES      = 6;      // simultaneous copies of each effect
constexpr float MUSIC_LEVEL = 0.45f;  // music sits under the effects
constexpr float MIN_GAP     = 0.06f;  // seconds before the same effect may restart

const char* const SFX_FILES[static_cast<int>(Sfx::Count)] = {
    "click.wav", "lane_select.wav", "place.wav", "enemy_hit.wav", "player_hurt.wav",
    "enemy_death.wav", "player_death.wav", "coin.wav", "beam.wav", "tornado.wav",
    "thunder.wav", "win.wav", "lose.wav"
};

struct Effect {
    ma_sound voice[VOICES];
    bool     loaded[VOICES] = {};
    int      next = 0;
    double   lastPlayed = -1.0;
};

ma_engine engine;
ma_sound  music;
bool      engineReady = false;
bool      musicReady  = false;
Effect    effects[static_cast<int>(Sfx::Count)];

float masterVolume = 0.7f;
bool  isMuted      = false;

double now() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

bool fileExists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return f.good();
}

// Finds the assets/audio folder whether the game is started from the project folder,
// the build folder, or by double-clicking the .exe.
std::string findAudioDir() {
    std::vector<std::string> dirs = {"assets/audio/", "../assets/audio/", "../../assets/audio/"};
#ifdef _WIN32
    char exe[MAX_PATH] = {};
    const DWORD count = GetModuleFileNameA(nullptr, exe, MAX_PATH);
    if (count > 0 && count < MAX_PATH) {
        std::string dir(exe);
        dir = dir.substr(0, dir.find_last_of("\\/"));
        dirs.insert(dirs.begin(), dir + "/assets/audio/");
    }
#endif
    for (const auto& d : dirs)
        if (fileExists(d + "temple_suspense.wav")) return d;
    return {};
}

void applyVolume() {
    if (engineReady) ma_engine_set_volume(&engine, isMuted ? 0.0f : masterVolume);
}

} // namespace

bool init() {
    if (engineReady) return true;

    const std::string dir = findAudioDir();
    if (dir.empty()) {
        std::cerr << "Audio: assets/audio folder not found, running without sound.\n";
        return false;
    }
    if (ma_engine_init(nullptr, &engine) != MA_SUCCESS) {
        std::cerr << "Audio: no audio device, running without sound.\n";
        return false;
    }
    engineReady = true;

    // Background music: streamed from disk and looped for the whole session.
    const std::string musicPath = dir + "temple_suspense.wav";
    if (ma_sound_init_from_file(&engine, musicPath.c_str(), MA_SOUND_FLAG_STREAM,
                                nullptr, nullptr, &music) == MA_SUCCESS) {
        musicReady = true;
        ma_sound_set_looping(&music, MA_TRUE);
        ma_sound_set_volume(&music, MUSIC_LEVEL);
        ma_sound_start(&music);
    }

    // Effects: decoded into memory once; several voices so they can overlap.
    for (int i = 0; i < static_cast<int>(Sfx::Count); ++i) {
        const std::string path = dir + SFX_FILES[i];
        for (int v = 0; v < VOICES; ++v) {
            effects[i].loaded[v] = ma_sound_init_from_file(
                &engine, path.c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr,
                &effects[i].voice[v]) == MA_SUCCESS;
        }
        if (!effects[i].loaded[0]) std::cerr << "Audio: missing " << path << "\n";
    }

    applyVolume();
    return true;
}

void shutdown() {
    if (!engineReady) return;
    for (Effect& e : effects)
        for (int v = 0; v < VOICES; ++v)
            if (e.loaded[v]) { ma_sound_uninit(&e.voice[v]); e.loaded[v] = false; }
    if (musicReady) { ma_sound_uninit(&music); musicReady = false; }
    ma_engine_uninit(&engine);
    engineReady = false;
}

void play(Sfx sfx) {
    if (!engineReady || sfx == Sfx::Count) return;
    Effect& e = effects[static_cast<int>(sfx)];
    if (!e.loaded[0]) return;

    const double t = now();
    if (e.lastPlayed >= 0.0 && t - e.lastPlayed < MIN_GAP) return;   // avoid ugly stacking
    e.lastPlayed = t;

    ma_sound& voice = e.voice[e.next];
    e.next = (e.next + 1) % VOICES;
    ma_sound_seek_to_pcm_frame(&voice, 0);
    ma_sound_start(&voice);
}

void setVolume(float v) {
    masterVolume = std::clamp(v, 0.0f, 1.0f);
    applyVolume();
}
float volume()      { return masterVolume; }
void  volumeUp()    { setVolume(masterVolume + 0.1f); }
void  volumeDown()  { setVolume(masterVolume - 0.1f); }

void  setMuted(bool m) { isMuted = m; applyVolume(); }
bool  muted()          { return isMuted; }
void  toggleMute()     { setMuted(!isMuted); }

} // namespace audio
