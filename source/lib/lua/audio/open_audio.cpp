#include "../pch.h"
#include "open.h"
#include "lib/audio/audio_manager.h"
#include "lib/json/node.h"

#include "resources/res_manager.h"
#include "lib/json/core_json.h"


using namespace ::radiant;
using namespace ::radiant::audio;
using namespace luabind;

void set_next_music_volume(int volume, std::string const& channel) {
   audio::AudioManager &a = audio::AudioManager::GetInstance();
   a.SetNextMusicVolume(volume, channel);
}

void set_next_music_fade(int fade, std::string const& channel) {
   audio::AudioManager &a = audio::AudioManager::GetInstance();
   a.SetNextMusicFade(fade, channel);
}

void set_next_music_loop(bool loop, std::string const& channel) {
   audio::AudioManager &a = audio::AudioManager::GetInstance();
   a.SetNextMusicLoop(loop, channel);
}

void play_music(std::string const& track, std::string const& channel) {
   audio::AudioManager &a = audio::AudioManager::GetInstance();
   a.PlayMusic(track, channel);
}

void play_sound(std::string const& uri) {
   audio::AudioManager &a = audio::AudioManager::GetInstance();
   a.PlaySound(uri);
}


//DEFINE_INVALID_JSON_CONVERSION(AudioManager)

void lua::audio::open(lua_State *L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("audio") [
            def("set_next_music_volume", &set_next_music_volume),
            def("set_next_music_fade", &set_next_music_fade),
            def("set_next_music_loop", &set_next_music_loop),
            def("play_music", &play_music),
            def("play_sound", &play_sound)
         ]
      ]
   ];
}