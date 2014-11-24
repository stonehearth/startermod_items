local AutotestAi = class()
local Deferred = class()

local NEXT_UNIQUE_ID = 1

function Deferred:__init(entity, unique_id)
   radiant.events.listen(entity, 'stonehearth:autotest:compel', function(e)
         if e.unique_id == unique_id then
            if self._done_cb then
               self._done_cb()
               self._done_cb = nil
            end
            return radiant.events.UNLISTEN
         end
      end)
end

function Deferred:done(done_cb)
   if self._finished then
      done_cb()
   else
      self._done_cb = done_cb
   end
end


function AutotestAi:__init(autotest)
	self._autotest = autotest
end

function AutotestAi:compel(entity, activity, args)
   -- make up a new action...
   local CompelAction = class()
   CompelAction.name = 'compelling ' .. activity
   CompelAction.does = 'stonehearth:top'
   CompelAction.args = {}
   CompelAction.version = 2
   CompelAction.priority = 1000 --- that's show em!

   local unique_id = NEXT_UNIQUE_ID
   NEXT_UNIQUE_ID = NEXT_UNIQUE_ID + 1
   
   local CompoundAction = stonehearth.ai:create_compound_action(CompelAction)
            :execute(activity, args)
            :execute('stonehearth:trigger_event', {
                  source = entity,
                  event_name = 'stonehearth:autotest:compel',
                  synchronous = true,
                  event_args = {
                     unique_id = unique_id
                  }
               })

   local ai_component = entity:add_component('stonehearth:ai')

   -- remove the action once the compelled action is done
   Deferred(entity, unique_id)
      :done(function()
            ai_component:remove_custom_action(CompoundAction)
         end)

   -- now inject it.
   ai_component:add_custom_action(CompoundAction)

   -- return a deferred for the client
   return Deferred(entity, unique_id);
end

return AutotestAi
