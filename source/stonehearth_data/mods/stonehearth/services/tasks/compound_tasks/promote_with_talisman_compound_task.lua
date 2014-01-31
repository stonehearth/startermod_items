local personality_service = require 'services.personality.personality_service'
local PromoteWithTalisman = class()

function PromoteWithTalisman:run(thread, args)
   local person = args.person
   local talisman = args.talisman

   stonehearth.ai:add_action(person, 'stonehearth:actions:grab_promotion_talisman')

   local talisman_component = talisman:get_component('stonehearth:promotion_talisman')
   local workshop = talisman_component:get_workshop():get_entity();

   local trigger_fn = function(info)
      if info.event == "change_outfit" then
         self:_change_profession(person, talisman)
      end
   end
   local args = {
      talisman = talisman,
      workshop = workshop,
      trigger_fn = trigger_fn,
   }
   thread:execute('stonehearth:grab_promotion_talisman', args)

   radiant.entities.destroy_entity(talisman)
   stonehearth.ai:remove_action(person, 'stonehearth:actions:grab_promotion_talisman')
end

function PromoteWithTalisman:_change_profession(person, talisman)
   --Remove the current class for the person
   local current_profession_script = person:get_component('stonehearth:profession'):get_script()
   local current_profession_script_api = radiant.mods.load_script(current_profession_script)
   if current_profession_script_api.demote then
      current_profession_script_api.demote(person)
   end

   --Add the new class
   local promotion_talisman_component = talisman:get_component('stonehearth:promotion_talisman')
   local script = promotion_talisman_component:get_script()
   radiant.mods.load_script(script).promote(person, promotion_talisman_component:get_workshop())

   --Log in personal event log
   local activity_name = radiant.entities.get_entity_data(talisman, 'stonehearth:activity_name')
   if activity_name then
      radiant.events.trigger(personality_service, 'stonehearth:journal_event', 
                             {entity = person, description = activity_name})
   end
end

return PromoteWithTalisman

