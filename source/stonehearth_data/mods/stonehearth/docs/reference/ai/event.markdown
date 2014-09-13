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

##Time

   - **hourly** - triggered every in-game hour by the calendar service. Passes args: {now = self._sv.date}
      - Key: stonehearth.calendar 
      - Name: stonehearth:midnight/sunrise/noon/sunset
      - Args: {now = current date}

   - **midnight** - triggers at midnight
   - **sunrise** - triggers at sunrise
   - **noon** - triggers at noon
   - **sunset** - triggers at sunset

For all the above events: 
   
   - Key: stonehearth.calendar 
   - Name: stonehearth:midnight/sunrise/noon/sunset
   - Args: none

Note: times of day (midnight, sunrise, noon, sunset are defined in calendar constants)
      "event_times" : {
         "midnight" : 0,
         "sunrise" : 6,
         "midday" : 14,
         "sunset_start" : 20,
         "sunset" : 22
      }

##Combat
   
   - **combat:battery** - triggered whenever a person is hit in combat. Triggered by _combat\_service.lua_: radiant.events.trigger_async(target, 'stonehearth:combat:battery', context)
      - Key: entity that was targeted
      - Name: stonehearth:combat:battery  
      - Args: context - some data about the nature of the attack
   - **kill_event** - triggered whenever a significant entity is killed and needs to inform its components etc that it is being destroyed. Only use if you are a component or an object related to that eneity, and must clean yourself up before the entity is destroyed. 
      - Key: entity that was killed
      - Name: stonehearth:kill_event
      - Args: none
      - Note: this is a synchronous event, so don't do anything crazy in the event handlers listening to it. On the flip side, you know the entity still exists. Please only use if you are a component on the entity and need to clean up after yourself. Otherwise, really complicated things can happen when a sync event goes off. 
   - **entity_killed** - triggered when an entity of significance (a villager, an enemy) dies
      - Key: radiant.entities 
      - Name: stonehearth:combat:entity_killed
      - Args: entity - the name, id, faction, and player_id of the entity that died
      - Note: this is async, so don't assume that the entity still exists. 


##Everyday Tasks
   
   - **sleep\_in\_bed** - triggered whenever we get up from sleeping in a bed, passes in entity that was sleeping
      - Key: entity that was sleeping
      - Name: stonehearth:sleep_in_bed
      - Args: none

   - **sleep\_on\_ground** - triggered when we get up from sleeping on the ground
      - Key: entity that was sleeping
      - Name: stonehearth:sleep_on_ground
      - Args: none