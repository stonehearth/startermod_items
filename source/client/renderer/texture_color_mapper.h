#pragma once
#include "types.h"
#include <unordered_map>

namespace radiant {
class TextureColorMapper {
public:
   static TextureColorMapper& GetInstance();

   ~TextureColorMapper();
   TextureColorMapper();

   std::pair<float, float> MapColor(const math3d::color3& c);
   H3DRes GetMaterial();
   H3DRes GetOverlay();

private:
   static      std::unique_ptr<TextureColorMapper> mapper_;
   H3DRes      texture_;
   H3DRes      material_;
   H3DRes      overlay_;

   std::unordered_map<int, std::pair<float, float>> uv_;
   int         size_;
};
}

