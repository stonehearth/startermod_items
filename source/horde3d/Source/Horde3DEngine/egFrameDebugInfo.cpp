// *************************************************************************************************
//
// Horde3D
//   Next-Generation Graphics Engine
// --------------------------------------
// Copyright (C) 2006-2011 Nicolas Schulz
//
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/legal/epl-v10.html
//
// *************************************************************************************************

#include "egFrameDebugInfo.h"
//#include "egModules.h"
//#include "egRenderer.h"

#include "utDebug.h"


namespace Horde3D {

using namespace std;

FrameDebugInfo::FrameDebugInfo() :
   samplingFrame_(false)
{
}

FrameDebugInfo::~FrameDebugInfo()
{

}

void FrameDebugInfo::sampleFrame()
{
   samplingFrame_ = true;
   reset();
}

void FrameDebugInfo::endFrame()
{
   samplingFrame_ = false;
}

void FrameDebugInfo::reset()
{
   shadowCascadeFrustums_.clear();
   directionalLights_.clear();
   splitFrustums_.clear();
}

void FrameDebugInfo::setViewerFrustum_(const Frustum& f)
{
   if (samplingFrame_)
   {
      sampledFrustum_ = f;
   }
}

void FrameDebugInfo::addDirectionalLightAABB_(const BoundingBox& b)
{
   if (samplingFrame_)
   {
      directionalLights_.push_back(b);
   }
}

void FrameDebugInfo::addShadowCascadeFrustum_(const Frustum& f)
{
   if (samplingFrame_)
   {
      shadowCascadeFrustums_.push_back(f);
   }
}

void FrameDebugInfo::addSplitFrustum_(const Frustum& f)
{
   if (samplingFrame_)
   {
      splitFrustums_.push_back(f);
   }
}

}  // namespace
