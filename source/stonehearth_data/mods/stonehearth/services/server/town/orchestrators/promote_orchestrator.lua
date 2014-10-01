local constants = require 'constants'

local Promote = class()

function Promote:run(town, args)
   local person = args.person
   local talisman = args.talisman
   
   local args = {
      trigger_fn = function(info, args)
         if info.event == "change_outfit" then
            local promotion_talisman_component = args.talisman:get_component('stonehearth:promotion_talisman')
            local job_uri = promotion_talisman_component:get_job()
            self:_change_job(person, job_uri)
            radiant.effects.run_effect(person, '/stonehearth/data/effects/level_up')
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

   elseif not talisman then
      --There is no talisman; we must be demoting back to worker. 
      --TODO: is there a different animation for demotion? If not just run effect
      self:_change_job(person, 'stonehearth:jobs:worker')

      --Find a decent cubemitter effect for demotion
      radiant.effects.run_effect(person, '/stonehearth/data/effects/level_up')
      return true
   else 
      -- talisman is an entity
      args.talisman = talisman

      result = town:command_unit(person, 'stonehearth:grab_promotion_talisman', args)
                           :once()
                           :start()
                           :wait()

      --TODO: what happens when a wildfire destroys the bench? See the clear_workshop orchestrator
   end

   if not result then
      return false
   end

   return true
end

function Promote:_change_job(person, job_uri)
   --Add the new class
   person:add_component('stonehearth:job'):promote_to(job_uri)
end

return Promote
