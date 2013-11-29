#ifndef _RADIANT_AUDIO_AUDIO_H
#define _RADIANT_AUDIO_AUDIO_H

#include "lib/json/node.h"

#define BEGIN_RADIANT_AUDIO_NAMESPACE  namespace radiant { namespace audio {
#define END_RADIANT_AUDIO_NAMESPACE    } }

BEGIN_RADIANT_AUDIO_NAMESPACE

class InputStream;

const int DEF_MUSIC_VOL = 50;
const int DEF_MUSIC_FADE = 1000; 
const int DEF_MUSIC_LOOP = true;

END_RADIANT_AUDIO_NAMESPACE

#endif //  _RADIANT_AUDIO_AUDIO_H
