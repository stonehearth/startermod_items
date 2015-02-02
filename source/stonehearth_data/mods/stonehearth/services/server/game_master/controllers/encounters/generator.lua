
local GeneratorEncounter = class()

function GeneratorEncounter:initialize()
   self._log = radiant.log.create_logger('game_master.encounters.generator')
end

function GeneratorEncounter:start(ctx, info)
   assert(info.delay)
   assert(info.spawn_edge)

   self._sv.ctx = ctx
   self._sv.spawn_edge = info.spawn_edge
   self.__saved_variables:mark_changed()

   local delay = info.delay
   local override = radiant.util.get_config('game_master.encounters.generator.delay')
   if override ~= nil then
      delay = override
   end

   self:_start_timer(delay)
end

function GeneratorEncounter:stop()
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
end

function GeneratorEncounter:_start_timer(delay)
   local ctx = self._sv.ctx
   self._log:info('setting generator timer for %s', tostring(delay))
   
   self._timer = stonehearth.calendar:set_timer(delay, function()
         ctx.arc:spawn_encounter(ctx, self._sv.spawn_edge)
         self:_start_timer(delay)
      end)
end

return GeneratorEncounter

