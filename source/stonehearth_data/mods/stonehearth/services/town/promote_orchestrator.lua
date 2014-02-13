local constants = require 'constants'
local personality_service = require 'services.personality.personality_service'
local PromoteWithTalisman = class()
local Point3 = _radiant.csg.Point3

function PromoteWithTalisman:run(thread, args)
   local person = args.person
   local talisman = args.talisman

   stonehearth.ai:add_action(person, 'stonehearth:actions:grab_promotion_talisman')

   local talisman_component = talisman:get_component('stonehearth:promotion_talisman')

   local trigger_fn = function(info)
      if info.event == "change_outfit" then
         self:_change_profession(person, talisman)
      end
   end
   local args = {
      talisman = talisman,
      trigger_fn = trigger_fn,
   }

   -- omg, so gross.  CompoundTask:execute() hides most of the task implementation, including
   -- priority.  That's... pretty dumb.   This entire system needs reworking!  -- tony
   local task = thread._scheduler:create_task('stonehearth:grab_promotion_talisman', args)
                                    :set_priority(constants.priorities.top.GRAB_PROMOTION_TALISMAN)
                                    :once()
   thread:_run(task)

   --They carry the saw invisibly for a block. How do avoid this?
   radiant.entities.remove_carrying(person)
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
   radiant.mods.load_script(script).promote(person)

   --Log in personal event log
   local activity_name = radiant.entities.get_entity_data(talisman, 'stonehearth:activity_name')
   if activity_name then
      radiant.events.trigger(personality_service, 'stonehearth:journal_event', 
                             {entity = person, description = activity_name})
   end
end

return PromoteWithTalisman

