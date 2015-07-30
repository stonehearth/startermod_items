local Entity = _radiant.om.Entity

--Allows you to spawn a target encounter at a time interval
--optionally, will allow you to set a max number of encounters
--If an out edge is specified, it will trigger when the max number of encounters is met
--optinally, will allow you to specify a source entity. If one is specified,
--the encounters will only be spawned if that source entity still exists. 

local GeneratorEncounter = class()

function GeneratorEncounter:initialize()
end

function GeneratorEncounter:restore()
end

function GeneratorEncounter:activate()
   self._log = radiant.log.create_logger('game_master.encounters.generator')

   if self._sv.source_entity then
      self:_start_source_listener()
   end
end

function GeneratorEncounter:start(ctx, info)
   assert(info.delay)
   assert(info.spawn_edge)

   self._sv.ctx = ctx
   self._sv.spawn_edge = info.spawn_edge
   self._sv.num_spawns = 0

   if info.source_entity then
      local entity = ctx:get(info.source_entity)
      if radiant.util.is_a(entity, Entity) and entity:is_valid() then
         self._sv.has_source = true
         self._sv.source_entity = entity
         self:_start_source_listener()
      else
         --we need a source entity, but one doesn't exist? abort encounter
         return
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
-- If we are have a max_spawns AND we have that many spawns, trigger the next encounter
-- Don't do anything if we're supposed to have a source entity, and it's missing.
function GeneratorEncounter:_spawn_encounter()
   local ctx = self._sv.ctx

   -- If we are supposed to have a source entity, and that source entity is not present, do not continue
   if self._sv.has_source then
      if not self._sv.source_entity or not self._sv.source_entity:is_valid() then
         self:_stop_timer()
         self.__saved_variables:mark_changed()
         return
      end
   end 
   
   self._log:info('spawning encounter at %s %s', stonehearth.calendar:format_time(), stonehearth.calendar:format_date())  
   ctx.arc:spawn_encounter(ctx, self._sv.spawn_edge)
   self._sv.num_spawns = self._sv.num_spawns + 1

   --If there is no "max number of spawns" or if there is and we've not yet exceeded it, spawn the encounter
   --otherwise, trigger the out edge
   if not self._sv.max_spawns or self._sv.num_spawns < self._sv.max_spawns then
      self:_start_timer()
   else
      local ctx = self._sv.ctx
      ctx.arc:trigger_next_encounter(ctx)
   end 

   self.__saved_variables:mark_changed()
end

function GeneratorEncounter:_stop_timer()
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
   end
end

function GeneratorEncounter:_start_timer()
   local delay = self._sv.delay
   
   self._log:info('setting generator timer for %s', tostring(delay))  

   self:_stop_timer()

   self._sv.timer = stonehearth.calendar:set_timer("GeneratorEncounter spawn", delay, radiant.bind(self, '_spawn_encounter'))
   self.__saved_variables:mark_changed()
end

function GeneratorEncounter:_start_source_listener()
   local source = self._sv.source_entity

   assert(source:is_valid())
   assert(not self._source_listener)

   self._source_listener = radiant.events.listen(source, 'radiant:entity:pre_destroy', function()
         self:_stop_timer()
         --I don't think killing the source should automatically start the out edge, if one is
         --specified. To do that, the right pattern is to listen on boss death/destroy and
         --handle appropriately
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

