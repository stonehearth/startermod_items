local Commands = class()
local InterruptAi = require 'mixintos.interrupt_ai.interrupt_ai'

function Commands:interrupt_ai(session, response, entity)
   stonehearth.ai:add_custom_action(entity, InterruptAi)
   radiant.events.listen(entity, 'stonehearth_debugtools:ai_interrupted', function()
         stonehearth.ai:remove_custom_action(entity, InterruptAi)
         return radiant.events.UNLISTEN
      end)
end

return Commands