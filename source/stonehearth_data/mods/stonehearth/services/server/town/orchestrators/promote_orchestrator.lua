local constants = require 'constants'

local Promote = class()

function Promote:run(town, args)
   local person = args.person
   local talisman = args.talisman

   local args = {
      talisman = talisman,
      trigger_fn = function(info)
         if info.event == "change_outfit" then
            self:_change_profession(person, talisman)
         end
      end
   }


   local result = town:command_unit(person, 'stonehearth:grab_promotion_talisman', args)
                        :once()
                        :start()
                        :wait()
   if not result then
      return false
   end

   --They carry the saw invisibly for a block. How do avoid this?
   radiant.entities.remove_carrying(person)
   radiant.entities.destroy_entity(talisman)
   return true
end

function Promote:_change_profession(person, talisman)
   --Add the new class
   local promotion_talisman_component = talisman:get_component('stonehearth:promotion_talisman')
   person:add_component('stonehearth:profession'):promote_to(promotion_talisman_component:get_profession())

   --Log in personal event log
   local activity_name = radiant.entities.get_entity_data(talisman, 'stonehearth:activity_name')
   if activity_name then
      radiant.events.trigger_async(stonehearth.personality, 'stonehearth:journal_event', 
                             {entity = person, description = activity_name})
   end
end

return Promote
