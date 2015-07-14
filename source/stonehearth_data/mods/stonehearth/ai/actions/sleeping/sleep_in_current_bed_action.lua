local SleepLib = require 'ai.lib.sleep_lib'
local Entity = _radiant.om.Entity

local SleepInCurrentBed = class()

SleepInCurrentBed.name = 'sleep in current bed'
SleepInCurrentBed.does = 'stonehearth:sleep_in_current_bed'
SleepInCurrentBed.status_text_key = 'ai_status_text_sleep_in_bed'
SleepInCurrentBed.args = {
   bed = Entity,
}
SleepInCurrentBed.version = 2
SleepInCurrentBed.priority = 1

function SleepInCurrentBed:start_thinking(ai, entity, args)
   local bed = args.bed
   local mount_component = bed:add_component('stonehearth:mount')
   if mount_component:get_user() == entity then
      ai:set_think_output()
   end
end

function SleepInCurrentBed:run(ai, entity, args)
   local bed = args.bed

   -- model variant should already be set. do it again for clarity.
   local bed_render_info = bed:add_component('render_info')
   bed_render_info:set_model_variant('sleeping')

   local sleep_duration, rested_sleepiness = SleepLib.get_sleep_parameters(entity, bed)
   ai:execute('stonehearth:run_sleep_effect', { duration_string = sleep_duration })
   radiant.entities.set_attribute(entity, 'sleepiness', rested_sleepiness)
end

function SleepInCurrentBed:stop(ai, entity, args)
   local bed = args.bed

   if bed:is_valid() then
      local mount_component = bed:add_component('stonehearth:mount')
      mount_component:dismount()
   end
end

return SleepInCurrentBed
