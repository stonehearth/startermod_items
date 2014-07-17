local PatrolHelpers = require 'ai.actions.patrol_helpers'
local OptimizedPathfinder = require 'ai.actions.optimized_pathfinder'
local Point3 = _radiant.csg.Point3
local Path = _radiant.sim.Path
local log = radiant.log.create_logger('town_patrol')

local GetPatrolPoint = class()
GetPatrolPoint.name = 'get patrol point'
GetPatrolPoint.does = 'stonehearth:get_patrol_route'
GetPatrolPoint.args = {}
GetPatrolPoint.think_output = {
   path = Path,
   patrollable_object = 'table',
}
GetPatrolPoint.version = 2
GetPatrolPoint.priority = 1

function GetPatrolPoint:start_thinking(ai, entity, args)
   self._entity = entity
   self._ai = ai
   self._start_location = ai.CURRENT.location
   self._listening = false

   self:_check_for_patrol_route()

   if not self._patrollable_object then
      self._listening = true
      radiant.events.listen(stonehearth.town_patrol, 'stonehearth:patrol_route_available', self, self._check_for_patrol_route)
   end
end

function GetPatrolPoint:stop_thinking(ai, entity, args)
   if self._listening then
      radiant.events.unlisten(stonehearth.town_patrol, 'stonehearth:patrol_route_available', self, self._check_for_patrol_route)
      self._listening = false
   end

   if self._proxy_entity then
      radiant.entities.destroy_entity(self._proxy_entity)
      self._proxy_entity = nil
   end

   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end

   self._patrollable_object = nil
   self._entity = nil
   self._ai = nil
end

function GetPatrolPoint:start(ai, entity, args)
   local proceed = stonehearth.town_patrol:mark_patrol_started(entity, self._patrollable_object)
   if not proceed then
      log:info('patrol route for entity %s is no longer valid', entity)
      -- in case ai:abort doesn't call stop_thinking
      self:stop_thinking(ai, entity, args)
      ai:abort()
   end
end

function GetPatrolPoint:_check_for_patrol_route()
   local patrol_margin = 2
   self._patrollable_object = stonehearth.town_patrol:get_patrol_route(self._entity)

   if self._patrollable_object then
      local waypoints = self._patrollable_object:get_waypoints(patrol_margin)
      self:_find_path(self._start_location, waypoints)

      self._listening = false
      return radiant.events.UNLISTEN
   end
end

-- calculate paths from each waypoint to the next
function GetPatrolPoint:_find_path(start_location, waypoints)
   local paths = {}
   local waypoint_index = 1
   local find_next_path -- "forward declaration" of function

   -- remove waypoints that this entity cannot stand on
   PatrolHelpers.prune_non_standable_points(self._entity, waypoints)

   -- find the "best" order of traversal
   PatrolHelpers.order_waypoints(self._entity, start_location, waypoints)

   -- the proxy target for the pathfinder, since the a* pathfinder requires an entity destination
   self._proxy_entity = radiant.entities.create_proxy_entity(false)

   -- doesn't matter where it we place it, we'll move it later
   radiant.terrain.place_entity_at_exact_location(self._proxy_entity, Point3.zero)

   local on_failure = function (message)
      log:info(message)
   end

   local on_success = function (path)
      -- record the solved path segment
      table.insert(paths, path)

      -- prepare for next segment
      local next_start_location = path:get_finish_point()
      waypoint_index = waypoint_index + 1

      if waypoint_index <= #waypoints then
         -- find path to the next waypoint
         find_next_path(next_start_location, waypoints[waypoint_index])
      else
         -- done! all paths solved
         self:_set_think_output(paths)
      end
   end

   find_next_path = function (start, finish)
      radiant.entities.move_to(self._proxy_entity, finish)
      self._pathfinder = OptimizedPathfinder(self._entity, self._proxy_entity, on_success, on_failure)
      self._pathfinder:set_start_location(start)
      self._pathfinder:start()
   end

   -- find the first path segment in the chain
   find_next_path(start_location, waypoints[waypoint_index])
end

function GetPatrolPoint:_set_think_output(paths)
   local combined_path = _radiant.sim.combine_paths(paths)
   self._ai:set_think_output({
      path = combined_path,
      patrollable_object = self._patrollable_object,
   })
end

return GetPatrolPoint
