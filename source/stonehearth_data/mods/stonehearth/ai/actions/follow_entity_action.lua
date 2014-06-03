local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()
local Entity = _radiant.om.Entity

local FollowEntity = class()

FollowEntity.name = 'follow entity'
FollowEntity.does = 'stonehearth:follow_entity'
FollowEntity.args = {
   target = Entity,

   follow_distance = {
      type = 'number',
      default = 3
   },

   settle_distance = {
      type = 'number',
      default = 2
   }
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
   local log = ai:get_log()
   local target = args.target
   local settle_distance = args.settle_distance

   local function check_distance()
      local distance = radiant.entities.distance_between(entity, target)
      if distance > args.follow_distance then 
         -- Note: this could pick a location off a cliff. Instead, maybe have the
         -- person run towards and stop an arbirtray distance from the target?
         local location = radiant.entities.pick_nearby_location(target, settle_distance)
         log:debug('starting to follow %s (distance: %.2f)', target, distance)
         ai:set_think_output({ location = location })
         if self._trace then
            self._trace:destroy()
            self._trace = nil
         end
         return true
      else
         log:detail('too close to follow %s (distance:%.2f). monitoring.', target, distance)
      end
   end

   local started = check_distance()
   if not started then
      assert(not self._trace)
      self._trace = radiant.entities.trace_location(target, 'find path to entity')
         :on_changed(function()
               check_distance()
            end)
         :on_destroyed(function()
               ai:abort('target destination destroyed')
            end)
   end
end

function FollowEntity:stop_thinking(ai, entity)
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(FollowEntity)
         :execute('stonehearth:goto_location', { location = ai.PREV.location })
