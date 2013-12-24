#ifndef _RADIANT_DM_TRACE_CATEGORIES_H
#define _RADIANT_DM_TRACE_CATEGORIES_H

BEGIN_RADIANT_DM_NAMESPACE

enum TraceCategories {
   OBJECT_MODEL_TRACES = 0,
   PATHFINDER_TRACES = 1,
   RENDER_TRACES = 2,
   RPC_TRACES = 3,
   LUA_SYNC_TRACES = 4,
   LUA_ASYNC_TRACES = 5,
   PHYSICS_TRACES = 6,
   PLAYER_1_TRACES = 1000,
};

END_RADIANT_DM_NAMESPACE

#endif //  _RADIANT_DM_TRACE_CATEGORIES_H
