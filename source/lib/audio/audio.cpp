#include "radiant.h"
#include "audio.h"
#include "audio_manager.h"
#include "lib/json/node.h"

using namespace ::radiant;
using namespace ::radiant::audio;

void audio::PlayUISound(std::string uri)
{
   audio::AudioManager &a =  audio::AudioManager::GetInstance();
   a.PlayUISound(uri);
}

void audio::PlayBGM(std::string uri)
{
}


json::Node audio::GetUIEFXSettings()
{
   json::Node n;
   return n;
}

void audio::SetUIEFXSettings()
{
}

json::Node audio::GetBGMSettings()
{
   json::Node n;
   return n;
}

void audio::SetBGMSettings()
{
}