#ifndef _RADIANT_CLIENT_RENDER_H3D_RESOURCE_TYPES_H
#define _RADIANT_CLIENT_RENDER_H3D_RESOURCE_TYPES_H

#include "namespace.h"
#include "core/unique_resource.h"
#include "Horde3D.h"
#include "Horde3DRadiant.h"

/*
 * These wrap the H3D* types with ref-counting (for shared) or unique resource
 * tracking classes from radiant::core.  Only use the naked horde types for
 * borrowed resources (e.g. to pass parameters around in the system).  The owner
 * of the resource (e.g. most of a class's instance variables) should always
 * use one of these resource types so the resource will automatically be reclaimed
 * in your destructor.
 */

BEGIN_RADIANT_CLIENT_NAMESPACE

typedef core::UniqueResource<H3DRes,  h3dUnloadResource> H3DResUnique;
typedef core::UniqueResource<H3DNode, h3dRemoveNode> H3DNodeUnique;
typedef core::UniqueResource<H3DNode, h3dRadiantStopCubemitterNode> H3DCubemitterNodeUnique;

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_H3D_RESOURCE_TYPES_H
