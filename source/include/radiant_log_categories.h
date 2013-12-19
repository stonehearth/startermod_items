#ifndef _RADIANT_LOG_CATEGORIES_H
#define _RADIANT_LOG_CATEGORIES_H

#define RADIANT_LOG_CATEGORIES \
   ADD_CATEGORY(browser) \
   ADD_CATEGORY(network) \
   ADD_CATEGORY(deferred) \
   ADD_CATEGORY(client) \
   ADD_CATEGORY(app) \
   ADD_CATEGORY(resources) \
   ADD_CATEGORY(analytics) \
   ADD_CATEGORY(audio) \
   ADD_CATEGORY(rpc) \
   \
   BEGIN_GROUP(lua) \
      ADD_CATEGORY(code) \
      ADD_CATEGORY(data) \
   END_GROUP(lua) \
   \
   BEGIN_GROUP(physics) \
      ADD_CATEGORY(collision) \
      ADD_CATEGORY(navgrid) \
   END_GROUP(physics) \
   \
   BEGIN_GROUP(csg) \
      ADD_CATEGORY(meshtools) \
   END_GROUP(csg) \
   \
   BEGIN_GROUP(core) \
      ADD_CATEGORY(config) \
      ADD_CATEGORY(guard) \
      ADD_CATEGORY(perf) \
   END_GROUP(core) \
   \
   BEGIN_GROUP(simulation) \
      ADD_CATEGORY(core) \
      ADD_CATEGORY(pathfinder) \
      ADD_CATEGORY(follow_path) \
      ADD_CATEGORY(goto_location) \
   END_GROUP(simulation) \
   \
   BEGIN_GROUP(horde) \
      ADD_CATEGORY(renderer) \
      ADD_CATEGORY(scene) \
   END_GROUP(horde) \
   \
   BEGIN_GROUP(renderer) \
      ADD_CATEGORY(carry_block) \
      ADD_CATEGORY(effects_list) \
      ADD_CATEGORY(entity) \
      ADD_CATEGORY(entity_container) \
      ADD_CATEGORY(lua_component) \
      ADD_CATEGORY(mob) \
      ADD_CATEGORY(render_info) \
      ADD_CATEGORY(terrain) \
      ADD_CATEGORY(renderer) \
      ADD_CATEGORY(animation) \
   END_GROUP(renderer) \
   \
   BEGIN_GROUP(om) \
      ADD_CATEGORY(entity) \
      ADD_CATEGORY(target_tables) \
      ADD_CATEGORY(destination) \
      ADD_CATEGORY(mob) \
   END_GROUP(om) \
   \
   BEGIN_GROUP(dm) \
      ADD_CATEGORY(store) \
      ADD_CATEGORY(streamer) \
      ADD_CATEGORY(receiver) \
      ADD_CATEGORY(trace) \
   END_GROUP(dm)


#endif // _RADIANT_LOG_CATEGORIES_H
