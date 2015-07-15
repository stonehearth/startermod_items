local SleepLib = require 'ai.lib.sleep_lib'

local SleepOnGroundAction = class()

SleepOnGroundAction.name = 'sleep on ground'
SleepOnGroundAction.status_text_key = 'ai_status_text_sleep_on_ground'
SleepOnGroundAction.does = 'stonehearth:sleep_exhausted'
SleepOnGroundAction.args = {}
SleepOnGroundAction.version = 2
SleepOnGroundAction.priority = 1

function SleepOnGroundAction:run(ai, entity, args)
   ai:execute('stonehearth:drop_carrying_now')
   ai:execute('stonehearth:run_effect', { effect = 'yawn' })
   ai:execute('stonehearth:run_effect', { effect = 'sit_on_ground' })

   local sleep_duration, rested_sleepiness = SleepLib.get_sleep_parameters(entity, nil)
   ai:execute('stonehearth:run_sleep_effect', { duration_string = sleep_duration })
   radiant.entities.set_attribute(entity, 'sleepiness', rested_sleepiness)

   local add_groggy_buff = true

   local attributes_component = entity:get_component('stonehearth:attributes')
   if attributes_component then
      local stamina = attributes_component:get_attribute('stamina')
      if stamina and stamina >= stonehearth.constants.attribute_effects.STAMINA_SLEEP_ON_GROUND_OKAY_THRESHOLD then
         add_groggy_buff = false
      end
   end

   if add_groggy_buff then
      radiant.entities.add_buff(entity, 'stonehearth:buffs:groggy')
   end

   radiant.events.trigger_async(entity, 'stonehearth:sleep_on_ground')
end

function SleepOnGroundAction:stop(ai, entity, args)
   -- xxx, localize
   local name = radiant.entities.get_display_name(entity)
   stonehearth.events:add_entry(name .. ' awakes groggy from sleeping on the cold, hard, unforgiving earth.', 'warning')
end

return SleepOnGroundAction
