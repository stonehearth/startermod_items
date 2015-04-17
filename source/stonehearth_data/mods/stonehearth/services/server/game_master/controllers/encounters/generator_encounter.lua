local Entity = _radiant.om.Entity

local GeneratorEncounter = class()

function GeneratorEncounter:initialize()
end

function GeneratorEncounter:restore()
end

function GeneratorEncounter:activate()
   self._log = radiant.log.create_logger('game_master.encounters.generator')
   self._sv.num_spawns = 0

   if self._sv.timer then
      self._sv.timer:bind(function()
            self:_spawn_encounter()
         end)         
   end
   if self._sv.source_entity then
      self:_start_source_listener()
   end
end

function GeneratorEncounter:start(ctx, info)
   assert(info.delay)
   assert(info.spawn_edge)

   self._sv.ctx = ctx
   self._sv.spawn_edge = info.spawn_edge

   if info.source_entity then
      local entity = ctx:get(info.source_entity)
      if radiant.util.is_a(entity, Entity) and entity:is_valid() then
         self._sv.source_entity = entity
         self:_start_source_listener()
      end
   end

   if info.max_spawns then
      self._sv.max_spawns = info.max_spawns
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
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
      self.__saved_variables:mark_changed()
   end
   if self._source_listener then
      self._source_listener:destroy()
      self._source_listener = nil
   end
end

-- Spawn the encounter associated with this generator
-- If no max-encounters have been specified, or if we aren't yet at max-encounters, 
-- start a timer for the next encounter
--If we are have a max_spawns AND we have that many spawns, trigger the next encounter
function GeneratorEncounter:_spawn_encounter()
   local ctx = self._sv.ctx
   
   self._log:info('spawning encounter at %s %s', stonehearth.calendar:format_time(), stonehearth.calendar:format_date())  
   ctx.arc:spawn_encounter(ctx, self._sv.spawn_edge)
   self._sv.num_spawns = self._sv.num_spawns + 1

   if not self._sv.max_spawns or self._sv.num_spawns < self._sv.max_spawns then
      self:_start_timer()
   else
      local ctx = self._sv.ctx
      ctx.arc:trigger_next_encounter(ctx)
   end 
end

function GeneratorEncounter:_start_timer()
   local delay = self._sv.delay
   
   self._log:info('setting generator timer for %s', tostring(delay))  

   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
   end

   self._sv.timer = stonehearth.calendar:set_timer(delay, function()
         self:_spawn_encounter()
      end)
   self.__saved_variables:mark_changed()
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

function GeneratorEncounter:get_progress_cmd(session, response)
   local progress = {}
   if self._sv.timer then
      progress.next_spawn_time = stonehearth.calendar:format_remaining_time(self._sv.timer)
   end
   return progress;
end

function GeneratorEncounter:trigger_now_cmd(session, response)
   local ctx = self._sv.ctx

   self._log:info('spawning encounter now as requested by ui')
   self:_spawn_encounter();
   return true
end

return GeneratorEncounter

