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

function FollowEntity:start_thinking(ai, entity, args)
   local log = ai:get_log()
   local target = args.target
   local settle_distance = args.settle_distance

   if not target:is_valid() then
      return
   end

   local check_distance = function()
      local distance = radiant.entities.distance_between(entity, target)
      if distance and distance > args.follow_distance then 
         -- TODO: ensure that placement point is topologically close to target (not down a cliff or across a long fence)
         local origin = radiant.entities.get_world_grid_location(target)
         local location = radiant.terrain.find_placement_point(origin, 1, settle_distance)
         log:debug('starting to follow %s (distance: %.2f)', target, distance)
         ai:set_think_output({ location = location })
         if self._trace then
            self._trace:destroy()
            self._trace = nil
         end
         return true
      else
         log:detail('too close to follow %s (distance:%.2f). monitoring.', target, distance)
         return false
      end
   end

   local started = check_distance()
   if not started then
      assert(not self._trace)
      self._trace = radiant.entities.trace_grid_location(target, 'find path to entity')
         :on_changed(function()
               check_distance()
            end)
         :on_destroyed(function()
               log:info('target destination destroyed')
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
