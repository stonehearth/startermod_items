local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()
local Entity = _radiant.om.Entity

local FollowEntity = class()

FollowEntity.name = 'follow entity'
FollowEntity.does = 'stonehearth:follow_entity'
FollowEntity.args = {
   target = Entity
}
FollowEntity.version = 2
FollowEntity.priority = 1

--[[
Only run this action if I have an object of obsession and if I'm far from them
]]
function FollowEntity:start_thinking(ai, entity, args)
   --If we don't have an object of obsession, pick one
   --local obsession_component = entity:add_component('stonehearth:obsession')
   --local target = obsession_component:get_obsession_target() 
   --if not target then
   --   target = self:_pick_target(entity)
   --end

   --If I'm within 3 of my target, trace the target's move so we get called
   --again on change. 
   local target = args.target
   local distance = radiant.entities.distance_between(entity, target)
   if distance <= 3 then 
      if not self._trace then
      self._trace = radiant.entities.trace_location(target, 'find path to entity')
      :on_changed(function()
         self:start_thinking(ai, entity, args)
      end)
      :on_destroyed(function()
         ai:abort('target destination destroyed')
      end)
      end
      return
   end

   --Otherwise, pick a location within 3 of my target and go there
   self._target_destination = self:_pick_nearby_location(target)
   ai:set_think_output()
end

function FollowEntity:stop_thinking(ai, entity)
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end


function FollowEntity:_pick_nearby_location(target)
   local target_location = radiant.entities.get_world_grid_location(target)
   local radius = 2
   local dx = rng:get_int(-radius, radius)
   local dz = rng:get_int(-radius, radius)
   local destination = Point3(target_location)
   destination.x = destination.x + dx
   destination.z = destination.z + dz
   return destination
end

function FollowEntity:run(ai, entity, args)
   --ai:execute('stonehearth:go_toward_location', { location = self._target_destination})
    ai:execute('stonehearth:goto_location', { location = self._target_destination})
end

return FollowEntity

--local ai = stonehearth.ai
--return ai:create_compound_action(FollowHumanWhenBored)
--         :execute('stonehearth:follow_human')
