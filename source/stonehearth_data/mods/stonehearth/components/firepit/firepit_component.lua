local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

radiant.mods.load('stonehearth')

local FirepitComponent = class()

function FirepitComponent:__init(entity, data_binding)
   radiant.check.is_entity(entity)
   self._entity = entity

   self._is_lit = false
   self._my_wood = nil
   self._light_task = nil

   radiant.events.listen('radiant:events:calendar:sunrise', self)
   radiant.events.listen('radiant:events:calendar:sunset', self)
end


function FirepitComponent:extend(json)
   -- not really...
   if json and json.effective_radius then
      self._effective_raidus = json.effective_radius
   end
end

--[[ At sunset, tell a worker to fill the fire with wood, and light it.
     At dawn, tell a worker to extingish the fire
     TODO: right now, we automatically populate with wood.
     TODO: what about cooking, for which the FirepitComponent would need to stay lit during the day?
     I suppose a cook could check if it's lit, and when it's not, fill it again.
     Note: this could have been modeled as an action, but the fire never
     really decides to have different kinds of behavior. It's pretty static.
]]
FirepitComponent['radiant:events:calendar:sunset'] = function (self)
   self:light_fire()
end

FirepitComponent['radiant:events:calendar:sunrise'] = function (self)
   self:extinguish()
end

function FirepitComponent:light_fire()
   self._my_wood = radiant.entities.create_entity('stonehearth:oak_log')
   radiant.entities.add_child(self._entity, self._my_wood, Point3(0, 0, 0))

   self._curr_fire_effect =
      radiant.effects.run_effect(self._entity, '/stonehearth/data/effects/firepit_effect')
end

--[[
function FirepitComponent:init_worker_task()
  if not self._light_task then
     --create the task to gather wood for and feed the fire
     local ws = radiant.mods.load('stonehearth').worker_scheduler
     local faction = self._entity:get_component('unit_info'):get_faction()
     local worker_scheduler = ws:get_worker_scheduler(session.faction)

  end
  self._light_task:start()

end
]]

--[[
TODO: make this work with the fire pit
function Firepit:init_worker_scheduler()
   local worker_mod = radiant.mods.require 'stonehearth_worker_class.stonehearth_worker_class'
   local faction = self._entity.get_component('unit_info').get_faction()
   local worker_scheduler = worker_mod.get_worker_scheduler(faction)

   -- Any worker that's not carrying anything will do...
   local not_carrying_fn = function (worker)
      return radiant.entities.get_carrying(worker) == nil
   end

   worker_scheduler:add_worker_task('chop_tree')
                  :set_worker_filter_fn(not_carrying_fn)
                  :add_work_object(tree)
                  :set_action('stonehearth:chop_tree')
                  :start()
   return true
end
]]

function FirepitComponent:extinguish()
   if self._my_wood then
      radiant.entities.remove_child(self._entity, self._my_wood)
      radiant.entities.destroy_entity(self._my_wood)
      self._my_wood = nil
      self._curr_fire_effect:stop()
   end
end

--- In radius of light
-- TODO: determine what radius means, right now it is passed in by the json file
-- @param target_entity The thing we're trying to figure out is in the radius of the firepit
-- @return true if we're in the radius of the light of the fire, false otherwise
function FirepitComponent:in_radius(target_entity)
  local target_location = Point3(radiant.entities.get_world_grid_location(target_entity))
  local world_bounds = self:_get_world_bounds()
  return world_bounds:contains(target_location)
end

function FirepitComponent:_get_world_bounds()
   local origin = Point3(radiant.entities.get_world_grid_location(self._entity))
   local bounds = Cube3(Point3(origin.x - self._effective_raidus, origin.y, origin.z - self._effective_raidus),
                        Point3(origin.x + self._effective_raidus, origin.y + 1, origin.z + self._effective_raidus))
   return bounds
end

return FirepitComponent
