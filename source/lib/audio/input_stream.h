#ifndef _RADIANT_AUDIO_INPUT_STREAM_H
#define _RADIANT_AUDIO_INPUT_STREAM_H

#include "audio.h"
#include <SFML/System.hpp>

BEGIN_RADIANT_AUDIO_NAMESPACE

class InputStream : public sf::InputStream
{
public:
   InputStream(std::string const& uri);
   ~InputStream();

   sf::Int64 read(void* data, sf::Int64 size) override;
   sf::Int64 seek(sf::Int64 position) override;
   sf::Int64 tell() override;
   sf::Int64 getSize() override;

private:
   std::shared_ptr<std::istream>    stream_;
};

END_RADIANT_AUDIO_NAMESPACE

#endif //  _RADIANT_AUDIO_INPUT_STREAM_H
