local constants = require 'constants'

local Promote = class()

function Promote:run(town, args)
   local person = args.person
   local talisman = args.talisman
   
   local args = {
      trigger_fn = function(info, args)
         if info.event == "change_outfit" then
            self:_change_profession(person, args.talisman)
         elseif info.event == "remove_talisman" then
            -- xx for now destroy the talisman. Eventually store it in the talisman component so we can bring it back when the civ is demoted
            radiant.entities.remove_carrying(person)
            radiant.entities.destroy_entity(args.talisman)            
         end
      end
   }

   local result
   
   -- talisman can be an entity, or a uri to an entity type
   if type(talisman) == 'string' then
      args.talisman_uri = talisman

      result = town:command_unit(person, 'stonehearth:grab_promotion_talisman_type', args)
                        :once()
                        :start()
                        :wait()

      --TODO: how to handle the case where the talisman has disappeared (and there will never be another?)

   else
      -- talisman is an entity
      args.talisman = talisman

      result = town:command_unit(person, 'stonehearth:grab_promotion_talisman', args)
                           :once()
                           :start()
                           :wait()

      --TODO: what happens when a wildfire destroys the bench? See the clear_workshop orchestrator
      --TODO: also, this crashes in the c++ effect layer before it even gets here. Take another look. 
   end


   if not result then
      return false
   end

   return true
end

function Promote:_change_profession(person, talisman)
   --Add the new class
   local promotion_talisman_component = talisman:get_component('stonehearth:promotion_talisman')
   person:add_component('stonehearth:profession'):promote_to(promotion_talisman_component:get_profession(), talisman:get_uri())

   --Log in personal event log
   local activity_name = radiant.entities.get_entity_data(talisman, 'stonehearth:activity_name')
   if activity_name then
      radiant.events.trigger_async(stonehearth.personality, 'stonehearth:journal_event', 
                             {entity = person, description = activity_name})
   end
end

return Promote
