#ifndef _RADIANT_AUDIO_AUDIO_H
#define _RADIANT_AUDIO_AUDIO_H

#include "lib/json/node.h"

#define BEGIN_RADIANT_AUDIO_NAMESPACE  namespace radiant { namespace audio {
#define END_RADIANT_AUDIO_NAMESPACE    } }

BEGIN_RADIANT_AUDIO_NAMESPACE

class InputStream;

void PlayUISound(std::string uri);
void PlayBGM(std::string uri);

json::Node GetUIEFXSettings();
void SetUIEFXSettings();

json::Node GetBGMSettings();
void SetBGMSettings();

END_RADIANT_AUDIO_NAMESPACE

#endif //  _RADIANT_AUDIO_AUDIO_H
