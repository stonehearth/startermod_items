#include "../pch.h"
#include "open.h"
#include "lib/audio/audio_manager.h"
#include "lib/json/node.h"

#include "resources/res_manager.h"
#include "lib/json/core_json.h"


using namespace ::radiant;
using namespace ::radiant::audio;
using namespace luabind;

//TODO: how to make this take a json node from LUA?
void play_music(std::string track, bool loop, int fade, int volume, std::string channel) {
   json::Node node;
   node.set("track", track);
   node.set("loop", loop);
   node.set("fade", fade);
   node.set("volume", volume);
   node.set("channel", channel);
   
   audio::AudioManager &a = audio::AudioManager::GetInstance();
   a.PlayMusic(node);
}

void play_sound(std::string uri) {
   audio::AudioManager &a = audio::AudioManager::GetInstance();
   a.PlaySound(uri);
}


//DEFINE_INVALID_JSON_CONVERSION(AudioManager)

void lua::audio::open(lua_State *L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("audio") [
            def("play_music", &play_music),
            def("play_sound", &play_sound)
         ]
      ]
   ];
}