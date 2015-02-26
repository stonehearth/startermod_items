local Entity = _radiant.om.Entity

local GeneratorEncounter = class()

function GeneratorEncounter:initialize()
   self._log = radiant.log.create_logger('game_master.encounters.generator')
end

function GeneratorEncounter:start(ctx, info)
   assert(info.delay)
   assert(info.spawn_edge)

   self._sv.ctx = ctx
   self._sv.spawn_edge = info.spawn_edge

   if info.source_entity then
      local entity = ctx[info.source_entity]
      if radiant.util.is_a(entity, Entity) and entity:is_valid() then
         self._sv.source_entity = entity
         self:_start_source_listener()
      end
   end

   local delay = info.delay
   local override = radiant.util.get_config('game_master.encounters.generator.delay')
   if override ~= nil then
      delay = override
   end
   self._sv.delay = delay
   self.__saved_variables:mark_changed()

   self:_spawn_encounter()
end

function GeneratorEncounter:stop()
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
   if self._source_listener then
      self._source_listener:destroy()
      self._source_listener = nil
   end
end

function GeneratorEncounter:_spawn_encounter()
   local ctx = self._sv.ctx

   self._log:info('spawning encounter')
   ctx.arc:spawn_encounter(ctx, self._sv.spawn_edge)
   self:_start_timer()
end

function GeneratorEncounter:_start_timer()
   local delay = self._sv.delay
   
   self._log:info('setting generator timer for %s', tostring(delay))  

   if self._timer then
      self._timer:destroy()
   end
   self._timer = stonehearth.calendar:set_timer(delay, function()
         self:_spawn_encounter()
      end)
end

function GeneratorEncounter:_start_source_listener()
   local source = self._sv.source_entity

   assert(source:is_valid())
   assert(not self._source_listener)

   self._source_listener = radiant.events.listen(source, 'radiant:entity:pre_destroy', function()
         local ctx = self._sv.ctx
         ctx.arc:trigger_next_encounter(ctx)
      end)
end

-- debug commands sent by the ui

function GeneratorEncounter:trigger_now_cmd(session, response)
   local ctx = self._sv.ctx

   self._log:info('spawning encounter now as requested by ui')
   self:_spawn_encounter();
   return true
end

return GeneratorEncounter

