#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_stockpile.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderStockpile::RenderStockpile(const RenderEntity& entity, om::LuaComponentPtr stockpile) :
   entity_(entity)
{
   H3DNode parent = entity_.GetNode();
   std::string name = h3dGetNodeParamStr(parent, H3DNodeParams::NameStr) + std::string(" stockile ");

   debugShape_ = h3dRadiantAddDebugShapes(parent, name.c_str());

   auto result = h3dRadiantCreateStockpileNode(parent, name);
   node_ = result.first;
   nodeObj_ = result.second;

   auto update = [=]() {
      UpdateStockpile(stockpile->ToJson());
   };
   stockpile->TraceObjectChanges("rendering stockpile", update);
   update();
}

RenderStockpile::~RenderStockpile()
{
}

void RenderStockpile::UpdateStockpile(JSONNode const& data)
{
   if (nodeObj_) {
      json::ConstJsonObject obj(data);
      
      if (obj.has("size")) {
         math3d::ibounds3 bounds(math3d::ipoint3::origin, math3d::ipoint3::origin);
         bounds._max.x += obj["size"][0].as_integer();
         bounds._max.z += obj["size"][0].as_integer();
         nodeObj_->UpdateShape(bounds);
      }
   }
   Renderer::GetInstance().LoadResources();
}
