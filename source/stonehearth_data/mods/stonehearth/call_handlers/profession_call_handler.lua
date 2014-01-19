local GrabTalismanAction = require 'ai.actions.grab_talisman_action'
local ProfessionCallHandler = class()

require 'services.tasks.compound_tasks.promote_with_talisman_compound_task'

-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.

function ProfessionCallHandler:grab_promotion_talisman(session, response, person, entity)
   local talisman
   local workshop_component = entity:get_component("stonehearth:workshop")
   if workshop_component then
      -- a workshop was pased in instead of a talisman. grab the talisman from the workshop
      talisman = workshop_component:get_talisman()
   else
      talisman = entity
   end

   -- xxx: hmm.  do we need an easier way to create "one shot" schedulers like this?
   local scheduler = stonehearth.tasks:create_scheduler()
                        :set_activity('stonehearth:top')
                        :join(person)

   local task = scheduler:create_task('stonehearth:tasks:promote_with_talisman', {
         person = person,
         talisman = talisman,
      })
      :start()
      
   radiant.events.listen(task, 'completed', function()
          stonehearth.tasks:destroy_scheduler(scheduler)
         return radiant.events.UNLISTEN
      end)

   return true
end

return ProfessionCallHandler