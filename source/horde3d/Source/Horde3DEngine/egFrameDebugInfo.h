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

#ifndef _egFrameDebugInfo_H_
#define _egFrameDebugInfo_H_

#include "egPrerequisites.h"
#include "egPrimitives.h"
#include <cstring>


namespace Horde3D {

class FrameDebugInfo
{
public:
   FrameDebugInfo();
	~FrameDebugInfo();

   void sampleFrame();
   void endFrame();
   const std::vector<Frustum>& getSplitFrustums() { return splitFrustums_; }
   const std::vector<Frustum>& getShadowCascadeFrustums() { return shadowCascadeFrustums_; }
   const std::vector<BoundingBox>& getDirectionalLightAABBs() { return directionalLights_; }
   const Frustum& getViewerFrustum() { return sampledFrustum_; }
   void reset();

private:
   void addSplitFrustum_(const Frustum& f);
   void addShadowCascadeFrustum_(const Frustum& f);
   void addDirectionalLightAABB_(const BoundingBox& b);
   void setViewerFrustum_(const Frustum& f);

   bool samplingFrame_;
   Frustum sampledFrustum_;

   std::vector<BoundingBox> directionalLights_;
   std::vector<Frustum> shadowCascadeFrustums_;
   std::vector<Frustum> splitFrustums_;
   

	friend class SceneManager;
	friend class Renderer;
};

}
#endif // _egFrameDebugInfo_H_
