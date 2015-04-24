#ifndef _RADIANT_LOG_CATEGORIES_H
#define _RADIANT_LOG_CATEGORIES_H

#define RADIANT_LOG_CATEGORIES \
   ADD_CATEGORY(browser) \
   ADD_CATEGORY(test) \
   ADD_CATEGORY(typeinfo) \
   ADD_CATEGORY(network) \
   ADD_CATEGORY(deferred) \
   BEGIN_GROUP(client) \
      ADD_CATEGORY(core) \
      ADD_CATEGORY(clock) \
   END_GROUP(client) \
   ADD_CATEGORY(app) \
   ADD_CATEGORY(resources) \
   ADD_CATEGORY(sysinfo) \
   ADD_CATEGORY(analytics) \
   ADD_CATEGORY(audio) \
   ADD_CATEGORY(rpc) \
   ADD_CATEGORY(script_host) \
   ADD_CATEGORY(convert) \
   \
   BEGIN_GROUP(lua) \
      ADD_CATEGORY(code) \
      ADD_CATEGORY(data) \
      ADD_CATEGORY(memory) \
   END_GROUP(lua) \
   \
   BEGIN_GROUP(physics) \
      ADD_CATEGORY(collision) \
      ADD_CATEGORY(navgrid) \
      ADD_CATEGORY(free_motion) \
      ADD_CATEGORY(sensor_tracker) \
   END_GROUP(physics) \
   \
   BEGIN_GROUP(csg) \
      ADD_CATEGORY(meshtools) \
      ADD_CATEGORY(region) \
      ADD_CATEGORY(churn) \
   END_GROUP(csg) \
   \
   BEGIN_GROUP(core) \
      ADD_CATEGORY(system) \
      ADD_CATEGORY(log) \
      ADD_CATEGORY(timer) \
      ADD_CATEGORY(config) \
      ADD_CATEGORY(guard) \
      ADD_CATEGORY(perf) \
   END_GROUP(core) \
   \
   BEGIN_GROUP(simulation) \
      ADD_CATEGORY(remote_client) \
      ADD_CATEGORY(core) \
      ADD_CATEGORY(jobs) \
      BEGIN_GROUP(pathfinder) \
         ADD_CATEGORY(bfs) \
         ADD_CATEGORY(astar) \
         ADD_CATEGORY(direct) \
      END_GROUP(pathfinder) \
      ADD_CATEGORY(follow_path) \
      ADD_CATEGORY(goto_location) \
      ADD_CATEGORY(free_motion) \
      ADD_CATEGORY(terrain) \
   END_GROUP(simulation) \
   \
   BEGIN_GROUP(horde) \
      ADD_CATEGORY(general) \
      ADD_CATEGORY(pbo) \
      ADD_CATEGORY(renderer) \
      ADD_CATEGORY(material) \
      ADD_CATEGORY(scene) \
   END_GROUP(horde) \
   \
   BEGIN_GROUP(renderer) \
      ADD_CATEGORY(ui_buffer) \
      ADD_CATEGORY(effects_list) \
      ADD_CATEGORY(entity) \
      ADD_CATEGORY(entity_container) \
      ADD_CATEGORY(lua_component) \
      ADD_CATEGORY(mob) \
      ADD_CATEGORY(render_info) \
      ADD_CATEGORY(terrain) \
      ADD_CATEGORY(renderer) \
      ADD_CATEGORY(animation) \
      ADD_CATEGORY(pipeline) \
      ADD_CATEGORY(render_node) \
   END_GROUP(renderer) \
   \
   BEGIN_GROUP(om) \
      ADD_CATEGORY(entity) \
      ADD_CATEGORY(region) \
      ADD_CATEGORY(destination) \
      ADD_CATEGORY(mob) \
      ADD_CATEGORY(data_store) \
      ADD_CATEGORY(effects_list) \
   END_GROUP(om) \
   \
   BEGIN_GROUP(dm) \
      ADD_CATEGORY(store) \
      ADD_CATEGORY(streamer) \
      ADD_CATEGORY(receiver) \
      ADD_CATEGORY(boxed) \
      ADD_CATEGORY(set) \
      ADD_CATEGORY(map) \
      ADD_CATEGORY(record) \
      BEGIN_GROUP(trace) \
         ADD_CATEGORY(boxed) \
         ADD_CATEGORY(set) \
         ADD_CATEGORY(map) \
         ADD_CATEGORY(record) \
         ADD_CATEGORY(buffered) \
      END_GROUP(trace) \
   END_GROUP(dm) \
   ADD_CATEGORY(protobuf) \


#endif // _RADIANT_LOG_CATEGORIES_H
