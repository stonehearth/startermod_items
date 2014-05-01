#ifndef _RADIANT_HORDE3D_STOCKPILE_H
#define _RADIANT_HORDE3D_STOCKPILE_H

#include "../../horde3d/Source/Horde3DEngine/egPrerequisites.h"
#include "../../horde3d/Source/Horde3DEngine/egScene.h"
#include "../extension.h"
#include "namespace.h"
#include "om/om.h"
#include "csg/cube.h"
#include "Horde3D.h"
#include "utMath.h"

BEGIN_RADIANT_HORDE3D_NAMESPACE

class DecalNode;

struct StockpileTpl : public Horde3D::SceneNodeTpl
{
   StockpileTpl(std::string const& name) :
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
	static void renderFunc(std::string const& shaderContext, std::string const& theClass, bool debugView,
		                    const Horde3D::Frustum *frust1, const Horde3D::Frustum *frust2,
                          Horde3D::RenderingOrder::List order, int occSet, int lodLevel);

   static bool InitExtension();

public:
   void UpdateShape(const csg::Cube3& bounds);
   bool checkIntersection( const Horde3D::Vec3f &rayOrig, const Horde3D::Vec3f &rayDir, Horde3D::Vec3f &intsPos, Horde3D::Vec3f &intsNorm ) const override;
   void onFinishedUpdate() override;

private:
	StockpileNode(const StockpileTpl &terrainTpl);
   
private:
   typedef std::pair<H3DNode, DecalNode*> Entry;
   std::vector<Entry>      corners_;
   std::vector<Entry>      edges_;
   Entry                   center_;
   Horde3D::BoundingBox    _bLocalBox;  // AABB in bobject space  
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
