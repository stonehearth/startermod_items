#include "../pch.h"
#include "open.h"
#include "lib/audio/audio_manager.h"
#include "lib/audio/track_info.h"
#include "lib/json/node.h"

#include "resources/res_manager.h"
#include "lib/json/core_json.h"


using namespace ::radiant;
using namespace ::radiant::audio;
using namespace luabind;

static void Audio_QueueMusic(const char* channel, audio::TrackInfo const& info)
{
   audio::AudioManager::GetInstance().QueueMusic(channel, info);
}

static void Audio_PlayMusic(const char* channel, audio::TrackInfo const& info)
{
   audio::AudioManager::GetInstance().PlayMusic(channel, info);
}


IMPLEMENT_TRIVIAL_TOSTRING(radiant::audio::TrackInfo)
DEFINE_INVALID_JSON_CONVERSION(radiant::audio::TrackInfo)

void lua::audio::open(lua_State *L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("audio") [
            def("play_music",                      &Audio_PlayMusic),
            def("queue_music",                     &Audio_QueueMusic),

            lua::RegisterType_NoTypeInfo<::radiant::audio::TrackInfo>("TrackInfo")
               .def(constructor<>())
               .def_readwrite("volume",            &radiant::audio::TrackInfo::volume)
               .def_readwrite("loop",              &radiant::audio::TrackInfo::loop)
               .def_readwrite("fade_in_duration",  &radiant::audio::TrackInfo::fadeInDuration)
               .def_readwrite("track",             &radiant::audio::TrackInfo::track)
        ]
      ]
   ];
}