local RunEffectAction = class()

local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

RunEffectAction.name = 'run effect'
RunEffectAction.does = 'stonehearth:run_effect'
RunEffectAction.args = {
   effect = 'string',  -- effect_name
   delay = {
      type = 'number',
      default = 0
   },
   trigger_fn = {
      type = 'function',
      default = stonehearth.ai.NIL,
   },
   args = {
      type = 'table',
      default = stonehearth.ai.NIL,
   },
   times = {
      type = 'number',
      default = 1,
   },
   facing_entity = {
      type = Entity,
      default = stonehearth.ai.NIL,
   },
   facing_point = {
      type = Point3,
      default = stonehearth.ai.NIL,
   }
}
RunEffectAction.version = 2
RunEffectAction.priority = 1

function RunEffectAction:run(ai, entity, args)
   local effect_name = args.effect
   -- create the effect and register a callback to resume the ai thread
   -- when it finishes.

   if args.facing_entity then
      radiant.entities.turn_to_face(entity, args.facing_entity)
   elseif args.facing_point then
      radiant.entities.turn_to_face(entity, args.facing_point)      
   end

   local times = args.times

   local log = ai:get_log()
   for i = 1, times do
      log:debug('starting new effect "%s"', effect_name)
      self._effect = radiant.effects.run_effect(entity, effect_name, args.delay, nil, args.args)
      self._effect:set_trigger_cb(args.trigger_fn)
      self._effect:set_finished_cb(function()
         if self._effect then
            log:debug('stopped effect "%s" and resuming', effect_name)
            self:_destroy_effect()
            ai:resume('effect %s finished', effect_name)
         end
      end)

      ai:suspend()
   end   
   self:_destroy_effect()
end

function RunEffectAction:_destroy_effect()
   if self._effect then
      self._effect:set_trigger_cb(nil)
                  :set_finished_cb(nil)
                  :stop()
      self._effect = nil
   end
end

function RunEffectAction:stop(ai, entity, args)
   self:_destroy_effect()
end

return RunEffectAction
