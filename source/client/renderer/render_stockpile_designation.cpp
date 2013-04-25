#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_stockpile_designation.h"
#include "om/components/stockpile_designation.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderStockpileDesignation::RenderStockpileDesignation(const RenderEntity& entity, om::StockpileDesignationPtr stockpile) :
   entity_(entity)
{
   H3DNode parent = entity_.GetNode();
   std::string name = h3dGetNodeParamStr(parent, H3DNodeParams::NameStr) + std::string(" stockile designation");

   debugShape_ = h3dRadiantAddDebugShapes(parent, name.c_str());

   auto result = h3dRadiantCreateStockpileNode(parent, name);
   node_ = result.first;
   nodeObj_ = result.second;

   traces_ += stockpile->TraceBounds("render stockpile",
                                     std::bind(&RenderStockpileDesignation::UpdateStockpile,
                                               this,
                                               om::StockpileDesignationRef(stockpile)));

   traces_ += stockpile->TraceAvailable("render stockpile available",
                                        std::bind(&RenderStockpileDesignation::UpdateAvailable,
                                                  this,
                                                  om::StockpileDesignationRef(stockpile)));

   traces_ += stockpile->TraceReserved("render stockpile reserved",
                                        std::bind(&RenderStockpileDesignation::UpdateReserved,
                                                  this,
                                                  om::StockpileDesignationRef(stockpile)));
   UpdateStockpile(stockpile);
   UpdateAvailable(stockpile);
   UpdateReserved(stockpile);
}

RenderStockpileDesignation::~RenderStockpileDesignation()
{
}

void RenderStockpileDesignation::UpdateStockpile(om::StockpileDesignationRef s)
{
   auto stockpile = s.lock();
   if (stockpile) {
      if (nodeObj_) {
         nodeObj_->UpdateShape(stockpile->GetBounds());
      }
      Renderer::GetInstance().LoadResources();
   }
}

void RenderStockpileDesignation::UpdateAvailable(om::StockpileDesignationRef s)
{
   if (entity_.ShowDebugRegions()) {
      auto stockpile = s.lock();
      if (stockpile) {
         h3dRadiantClearDebugShape(debugShape_);
         h3dRadiantAddDebugRegion(debugShape_, stockpile->GetAvailableRegion(), math3d::color4(92, 92, 92, 192));
         h3dRadiantCommitDebugShape(debugShape_);
      }
   }
}

void RenderStockpileDesignation::UpdateReserved(om::StockpileDesignationRef s)
{
   auto stockpile = s.lock();
   if (stockpile) {
   }
}
