# Overview

Events are triggered when key things happen in the game. A mod can listen to game events to have behavior happen at appropriate moments. 

To trigger an event, call radiant.events.trigger_async and pass in an identifying key (usually the entity that the event concerns; if it's a general event, sometimes the service that relates to that event), the name of the event (namespaced to the mod in question) and an optional set of arguments: 

      radiant.events.trigger_async(entity, 'stonehearth:admiring_fire', {})

Note, the above function triggers events async. So when you get the event, you should confirm the worldstate. You cannot, for example, assume that objects that existed when the event was fired still exist in the wold. 

To listen on an event, call radiant.events.listen and pass in the identifying key (should match the one that was passed into the trigger), the name of the event, and a function that should recieve the arguments and be called when the event is triggered. For example: 

      radiant.events.listen(entity, 'stonehearth:buff_added', function(e)
            --e has the arguments passed into the trigger
            if e.uri == uri then
               return cb()
            end
         end)

Or you can pass in a function that exists elsewhere in the class: 

      self._attribute_listener = radiant.events.listen(
         entity,
         'stonehearth:foo_event',
         self, self._on_attribute_changed)

      function ClassName:_on_attribute_changed()
         --do stuff
      end


If you no longer want to listen to an event, you can either return radiant.events.UNLISTEN in the callback function, or you can save the event and destroy it later. 

Unlistening from the callback: 

      function ClassName:_on_attribute_changed()
         radiant.events.UNLISTEN
      end

Destroying an event, given that it was saved to a function, as above: 

      if self._attribute_listener then
         self._attribute_listener:destroy()
         self._attribute_listener = nil
      end

#List of Events

All events listed below are prefaced in code by stonehearth:, because they are all part of the stonehearth mod. TODO: make each of these their own page?

- combat:battery - triggered whenever a person is hit in combat. Triggered by _combat\_service.lua_: radiant.events.trigger_async(target, 'stonehearth:combat:battery', context)