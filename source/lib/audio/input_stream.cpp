#include "radiant.h"
#include "input_stream.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::audio;

InputStream::InputStream(std::string const& uri)
{
   stream_ = res::ResourceManager2::GetInstance().OpenResource(uri);
}

InputStream::~InputStream()
{
}

sf::Int64 InputStream::read(void* data, sf::Int64 size) 
{
   stream_->read((char *)data, size);
   return size;
}

sf::Int64 InputStream::seek(sf::Int64 position)
{
   stream_->seekg(position);
   return tell();
}

sf::Int64 InputStream::tell()
{
   return static_cast<sf::Int64>(stream_->tellg());
}

sf::Int64 InputStream::getSize()
{
   sf::Int64 position = tell();
   stream_->seekg(0, std::ios_base::end);
   sf::Int64 size = tell();
   seek(position);
   return size;
}


