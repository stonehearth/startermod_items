#include "pch.h"
#include "texture_color_mapper.h"
#include "renderer.h"

using namespace ::radiant;
using namespace ::radiant::client;

TextureColorMapper& TextureColorMapper::GetInstance()
{
   if (!mapper_) {
      new TextureColorMapper();
      assert(mapper_);
   }
   return *mapper_;
}

TextureColorMapper::~TextureColorMapper()
{
}

TextureColorMapper::TextureColorMapper()
{
   assert(!mapper_);
   mapper_.reset(this);

   overlay_ = 0;
   size_ = 32;

   ostringstream name;
   name << "GridTexture ";

   texture_ = h3dCreateTexture(name.str().c_str(), size_, size_, H3DFormats::List::TEX_BGRA8, H3DResFlags::NoTexMipmaps);
   assert(texture_);
   unsigned char *data = (unsigned char *)h3dMapResStream(texture_, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);
   for (int i = 0; i < size_; i++) {
      for(int j = 0; j < size_; j++) {
         unsigned char *p = data + ((j * size_) + i) * 4;
         *p++ = (unsigned char)(255.0 * i / size_);
         *p++ = (unsigned char)(255.0 * j / size_);
         *p++ = 0xff;
         *p++ = 0xff;
      }
   }
   h3dUnmapResStream(texture_);

   // Create a new material.  This is pretty dumb...
   name << " Material";
   material_ = h3dAddResource( H3DResTypes::Material, name.str().c_str(), 0 ); // we need the material name
   if ( !material_  )
      ASSERT(false);

   // Setup the XML std::string to represent the material data
   std::string materialData;
   if (texture_)
   {
      std::string textureName = h3dGetResName( texture_ );                        // we need the texture resource, if it exists

      materialData.append("<Material>\n");
	   materialData.append("   <Shader source=\"shaders/model.shader\" />\n");
	   materialData.append("   <Sampler name=\"albedoMap\" map=\"" + textureName + "\" />\n");
      materialData.append("</Material>\n");
   }

   // Load the material resource from the material data string
   bool result = h3dLoadResource( material_, materialData.c_str(), materialData.length() );
   assert(result);
   Renderer::GetInstance().LoadResources();
}

std::pair<float, float> TextureColorMapper::MapColor(const math3d::color3& c)
{
   int hash = (c.r << 16) | (c.g << 8) | c.b;
   auto i = uv_.find(hash);
   if (i != uv_.end()) {
      return i->second;
   }

   unsigned char *data = (unsigned char *)h3dMapResStream(texture_, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, true, true);
   assert(data);
   int offset = uv_.size();
   int x = offset % size_;
   int y = offset / size_;
   unsigned char *color = data + ((y * size_) + x) * 4;
   *color++ = c.b;
   *color++ = c.g;
   *color++ = c.r;
   *color++ = 0xff;
   h3dUnmapResStream(texture_);
   auto coord = [&](int c)->float {
      return (((float)c + 0.5f) / (float)size_);
   };
   auto result = std::make_pair(coord(x), coord(size_ - y - 1));
   uv_[hash] = result;
   return result;
}

H3DRes TextureColorMapper::GetMaterial()
{
   return material_;
}

H3DRes TextureColorMapper::GetOverlay()
{
   if (!overlay_) {
      ASSERT(texture_);

      overlay_ = h3dAddResource( H3DResTypes::Material, "TextureColorMapper overlay", 0 );
      ASSERT(overlay_);

      std::string textureName = h3dGetResName( texture_ );                        // we need the texture resource, if it exists
      std::string materialData;
      materialData.append("<Material>\n");
	   materialData.append("   <Shader source=\"shaders/overlay.shader\" />\n");
	   materialData.append("   <Sampler name=\"albedoMap\" map=\"" + textureName + "\" />\n");
      materialData.append("</Material>\n");

      // Load the material resource from the material data string
      bool result = h3dLoadResource(overlay_, materialData.c_str(), materialData.length());
      ASSERT(result);
   }
   return overlay_;
}
