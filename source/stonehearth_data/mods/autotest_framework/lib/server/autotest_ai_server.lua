local AutotestAi = class()
local Deferred = class()

function Deferred:__init(entity, activity)
   radiant.events.listen(entity, 'stonehearth:autotest:compel', function(e)
         if e.activity == activity then
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

   local CompoundAction = stonehearth.ai:create_compound_action(CompelAction)
            :execute(activity, args)
            :execute('stonehearth:trigger_event', {
                  source = entity,
                  event_name = 'stonehearth:autotest:compel',
                  synchronous = true,
                  event_args = {
                     activity = activity
                  }
               })

   local ai_component = entity:add_component('stonehearth:ai')

   -- remove the action once the compelled action is done
   Deferred(entity, activity)
      :done(function()
            ai_component:remove_custom_action(CompoundAction)
         end)

   -- now inject it.
   ai_component:add_custom_action(CompoundAction)

   -- return a deferred for the client
   return Deferred(entity, activity);
end

return AutotestAi
