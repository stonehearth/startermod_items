local constants = require 'constants'
local Point3 = _radiant.csg.Point3
local log = radiant.log.create_logger('combat')

local TownDefenseObserver = class()

local Waypoint = class()

function Waypoint:__init(entity)
   self.entity = entity
   self.location = self:_get_centroid(entity)
   self.distance = 0
   self.visited = false
end

function Waypoint:_get_centroid(entity)
   local stockpile_component = entity:get_component('stonehearth:stockpile')
   local centroid

   if stockpile_component ~= nil then
      local bounds = stockpile_component:get_bounds()
      centroid = (bounds.min + bounds.max):scaled(0.5)
   else
      centroid = radiant.entities.get_world_grid_location(entity)
   end

   return centroid
end

function TownDefenseObserver:initialize(entity, json)
   self._entity = entity
   self._waypoints = {}
   self._last_waypoint = nil

   local player_id = entity:add_component('unit_info'):get_player_id()
   local inventory = stonehearth.inventory:get_inventory(player_id)
   local all_storage = inventory:get_all_storage()

   for id, storage in pairs(all_storage) do
      table.insert(self._waypoints, Waypoint(storage))
   end

   radiant.events.listen(inventory, 'stonehearth:storage_added', self, self._on_storage_added)
   radiant.events.listen(inventory, 'stonehearth:storage_removed', self, self._on_storage_removed)
   radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._on_poll)

   self:_check_issue_task()
end

function TownDefenseObserver:_on_poll()
   self:_check_issue_task()
end

function TownDefenseObserver:_check_issue_task()
   if self._patrol_task and not self._patrol_task:is_completed() then
      return
   end

   local waypoint = self:_get_next_waypoint()

   if waypoint then
      self:_create_patrol_task(waypoint)
   end
end

function TownDefenseObserver:_on_storage_added(args)
   local storage = args.storage

   if storage ~= nil and storage:is_valid() then
      table.insert(self._waypoints, Waypoint(storage))
   end
end

function TownDefenseObserver:_on_storage_removed(args)
   local storage = args.storage

   -- if storage is already invalid, the entry will get cleaned up in sort_waypoints
   for index, waypoint in pairs(self._waypoints) do
      if waypoint.entity == storage then
         table.remove(self._waypoints, index)
         return
      end
   end
end

function TownDefenseObserver:destroy()
   radiant.events.unlisten(stonehearth.inventory, 'stonehearth:storage_added', self, self._on_storage_added)
   radiant.events.unlisten(stonehearth.inventory, 'stonehearth:storage_removed', self, self._on_storage_removed)
   radiant.events.unlisten(radiant, 'stonehearth:very_slow_poll', self, self._on_poll)
end

function TownDefenseObserver:_create_patrol_task(waypoint)
   local function on_task_completed()
      self:_check_issue_task()
   end

   self._patrol_task = self._entity:get_component('stonehearth:ai')
                                   :get_task_group('stonehearth:work')
                                   :create_task('stonehearth:patrol', { waypoint = waypoint.entity })
                                   --:set_priority(constants.priorities.TBD)
                                   :once()
                                   :notify_completed(on_task_completed)
                                   :start()
   waypoint.visited = true
   self._last_waypoint = waypoint
end

function TownDefenseObserver:_get_next_waypoint()
   self:_sort_waypoints()

   for _, waypoint in pairs(self._waypoints) do
      if not waypoint.visited then
         -- could also check the lease
         return waypoint
      end
   end
   return nil
end

function TownDefenseObserver:_sort_waypoints()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local has_unvisited_waypoint = false

   for index, waypoint in pairs(self._waypoints) do
      -- prune invalid waypoints
      if not waypoint.entity:is_valid() then
         -- safe to remove key during pairs iteration
         table.remove(self._waypoints, index)
      end

      -- check if we've visited everything
      if not waypoint.visited then
         has_unvisited_waypoint = true
      end

      waypoint.distance = entity_location:distance_to(waypoint.location)
   end

   -- if all visited, reset all the visited flags
   if not has_unvisited_waypoint then
      for _, waypoint in pairs(self._waypoints) do
         -- don't visit the one we last visited
         -- need better logic for this
         if waypoint ~= self._last_waypoint then
            waypoint.visited = false
         end
      end
   end

   -- finally sort by increasing distance
   table.sort(self._waypoints,
      function (a, b)
         return a.distance < b.distance
      end
   )
end

return TownDefenseObserver
