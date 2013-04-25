#ifndef _RADIANT_HORDE3D_STOCKPILE_H
#define _RADIANT_HORDE3D_STOCKPILE_H

#include "../../horde3d/Source/Horde3DEngine/egPrerequisites.h"
#include "../../horde3d/Source/Horde3DEngine/egScene.h"
#include "../extension.h"
#include "math3d.h"
#include "namespace.h"
#include "om/om.h"
#include "Horde3D.h"

BEGIN_RADIANT_HORDE3D_NAMESPACE

class DecalNode;

struct StockpileTpl : public Horde3D::SceneNodeTpl
{
   StockpileTpl(const std::string &name) :
      Horde3D::SceneNodeTpl(SNT_StockpileNode, name)
   {
   }
};

class StockpileNode : public Horde3D::SceneNode
{
public:
	~StockpileNode();

	static Horde3D::SceneNodeTpl *parsingFunc(std::map< std::string, std::string > &attribs);
	static Horde3D::SceneNode *factoryFunc(const Horde3D::SceneNodeTpl &nodeTpl);
	static void renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
		                    const Horde3D::Frustum *frust1, const Horde3D::Frustum *frust2,
                          Horde3D::RenderingOrder::List order, int occSet);

   static bool InitExtension();

public:
   void UpdateShape(const math3d::ibounds3& bounds);

private:
	StockpileNode(const StockpileTpl &terrainTpl);
   
private:
   typedef std::pair<H3DNode, DecalNode*> Entry;
   std::vector<Entry>      corners_;
   std::vector<Entry>      edges_;
   Entry                   center_;
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
