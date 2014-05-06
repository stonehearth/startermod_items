local Spawner = class()
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

function Spawner:__init()
end

function Spawner:initialize(properties)
   radiant.events.listen(stonehearth.object_tracker, 'stonehearth:promote', self, self._on_promote)
end

function Spawner:_on_promote(e)
   local promoted_entity = e.entity
   if promoted_entity:get_component('stonehearth:profession'):get_profession_id() == 'carpenter' then


      local explored_regions = stonehearth.terrain:get_explored_region('civ'):get()
      local region_bounds = explored_regions:get_bounds()

      local spawn_point = Point2(region_bounds.max.x + 1, 1 + (region_bounds.max.y + region_bounds.min.y) / 2)


      local tombstone = radiant.entities.create_entity('stonehearth:tombstone')
      radiant.entities.set_name(tombstone, 'title')
      radiant.entities.set_description(tombstone, 'description')
      radiant.terrain.place_entity(tombstone, Point3(spawn_point.x, 1, spawn_point.y))
      return radiant.events.UNLISTEN
   end
end

return Spawner
