local SleepLib = require 'ai.lib.sleep_lib'
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local SleepInBedAdjacent = class()
SleepInBedAdjacent.name = 'sleep in bed adjacent'
SleepInBedAdjacent.status_text = 'sleeping...'
SleepInBedAdjacent.does = 'stonehearth:sleep_in_bed_adjacent'
SleepInBedAdjacent.args = {
   bed = Entity,     -- the bed to sleep in
}
SleepInBedAdjacent.version = 2
SleepInBedAdjacent.priority = 1

function SleepInBedAdjacent:start(ai, entity, args)
   --[[

   for the moment, let's not put leases on beds.  in the future we'll want
   to support ownership of items, which will probably be done with leases,
   so leave this code hanging around.  for now, though, treat beds as
   communal objects.
   
   local bed = args.bed
   local lease_component = bed:add_component('stonehearth:lease')
   if not lease_component:acquire('stonehearth:bed', entity) then
      ai:abort('could not lease %s (%s has it).', tostring(bed),
               tostring(lease_component:get_owner('stonehearth:bed')))
   end
   ]]
end

--[[
   Follow the path to the bed, then play the sleep-related animations.
--]]
function SleepInBedAdjacent:run(ai, entity, args)
   local bed = args.bed
   
   ai:execute('stonehearth:run_effect', { effect = 'yawn' })

   local mob = entity:get_component('mob')
   local offset = self:_get_sleep_offset(bed)

   self._in_bed = true
   self._saved_location = mob:get_world_grid_location()
   self._saved_facing = mob:get_facing()
   radiant.entities.add_child(bed, entity, offset, true)

   ai:execute('stonehearth:run_effect', { effect = 'goto_sleep' })

   local bed_render_info = bed:add_component('render_info')
   bed_render_info:set_model_variant('sleeping')

   radiant.events.trigger_async(entity, 'stonehearth:sleep_in_bed', { bed_uri = bed:get_uri() })

   local sleep_duration, rested_sleepiness = SleepLib.get_sleep_parameters(entity, bed)
   ai:execute('stonehearth:run_sleep_effect', { duration_string = sleep_duration })
   radiant.entities.set_attribute(entity, 'sleepiness', rested_sleepiness)
end

function SleepInBedAdjacent:stop(ai, entity, args)
   -- If we are saved while sleeping, we make the (dangerous) assumption that the task will restart and win on load.
   -- In this case, this action will run again and clean up after itself in stop.
   if self._in_bed then
      local root_entity = radiant.entities.get_root_entity()
      local mob = entity:get_component('mob')
      local egress_facing = self._saved_facing + 180

      radiant.entities.add_child(root_entity, entity, self._saved_location)
      mob:turn_to(egress_facing)

      local bed = args.bed
      if bed then
         local bed_render_info = bed:add_component('render_info')
         bed_render_info:set_model_variant('')
      end

      self._saved_location = nil
      self._saved_facing = nil
      self._in_bed = false
   end
end

function SleepInBedAdjacent:_get_sleep_offset(bed)
   -- TODO: read this from the bed's entity data
   local bed_facing = radiant.entities.get_facing(bed)
   local offset = Point3(0, 1, -0.75)
   offset = radiant.math.rotate_about_y_axis(offset, bed_facing)
   return offset
end

return SleepInBedAdjacent
