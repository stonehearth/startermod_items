
local SleepOnGroundAction = class()
SleepOnGroundAction.name = 'sleep on the cold, hard, unforgiving ground'
SleepOnGroundAction.does = 'stonehearth:sleep_exhausted'
SleepOnGroundAction.args = {}
SleepOnGroundAction.version = 2
SleepOnGroundAction.priority = 1

function SleepOnGroundAction:run(ai, entity)
   ai:execute('stonehearth:drop_carrying_now')
   ai:execute('stonehearth:run_effect', { effect = 'yawn' })
   ai:execute('stonehearth:run_effect', { effect = 'sit_on_ground' })

   -- goto sleep
   radiant.entities.add_buff(entity, 'stonehearth:buffs:sleeping');

   local sleep_duration = 1
   local attributes_component = entity:get_component('stonehearth:attributes')
   
   if attributes_component then
      local sleepiness = attributes_component:get_attribute('sleepiness')
      local stamina = attributes_component:get_attribute('stamina')
      local sleep_duration_attribute = attributes_component:get_attribute('sleep_duration')
      if sleep_duration_attribute then
         sleep_duration = sleep_duration_attribute
      end
   end

   sleep_duration = sleep_duration * 2
   sleep_duration = radiant.math.round(sleep_duration * 10) / 10

   local sleep_duration_string = sleep_duration .. 'h'

   ai:execute('stonehearth:run_effect_timed', { effect = 'sleep', duration = sleep_duration_string})
   radiant.entities.set_attribute(entity, 'sleepiness', stonehearth.constants.sleep.MIN_SLEEPINESS)

   local add_groggy_buff = true
   if attributes_component then
      local stamina = attributes_component:get_attribute('stamina')
      if stamina then
         if stamina >= stonehearth.constants.attribute_effects.STAMINA_SLEEP_ON_GROUND_OKAY_THRESHOLD then
            add_groggy_buff = false
         end
      end
   end

   if add_groggy_buff then
      radiant.entities.add_buff(entity, 'stonehearth:buffs:groggy')
   end

   radiant.events.trigger_async(entity, 'stonehearth:sleep_on_ground')
end

function SleepOnGroundAction:stop(ai, entity)
   -- xxx, localize
   local name = radiant.entities.get_display_name(entity)
   stonehearth.events:add_entry(name .. ' awakes groggy from sleeping on the cold, hard, unforgiving earth.', 'warning')
   radiant.entities.remove_buff(entity, 'stonehearth:buffs:sleeping');   
end

return SleepOnGroundAction

