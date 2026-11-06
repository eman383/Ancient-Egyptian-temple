#pragma once

// Game audio: looping background music + sound effects, with a master volume and mute.
// Built on miniaudio (external/miniaudio, single header, public domain / MIT-0).
// Every function is safe to call before init() or when no audio device exists:
// it simply does nothing, so the game and the automated tests never depend on sound.
namespace audio {

enum class Sfx {
    Click,        // menu buttons
    LaneSelect,   // lane toggled
    Place,        // player placed on a lane
    EnemyHit,     // a player's strike/arrow lands on an enemy
    PlayerHurt,   // an enemy attack lands on a player
    EnemyDeath,
    PlayerDeath,
    Coin,         // collectible picked up
    Beam,         // Medusa's eye lasers
    Tornado,      // Mummy's sand tornado
    Thunder,      // Pharaoh's lightning strike
    Win,
    Lose,
    Count
};

bool  init();         // call once after the window exists; false = no sound (game still runs)
void  shutdown();

void  play(Sfx sfx);

// Master volume 0..1 applied to music AND effects.
void  setVolume(float v);
float volume();
void  volumeUp();     // +10%
void  volumeDown();   // -10%

void  setMuted(bool m);
bool  muted();
void  toggleMute();

} // namespace audio
