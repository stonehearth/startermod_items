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
   
   - **combat:begin\_melee\_attack** - triggered whenever someone attacks another person in combat.
      - Key: entity that is attacking
      - Name: stonehearth:combat:begin\_melee\_attack
      - Args: the combat context
   - **combat:melee\_attack\_connect** - triggered whenever an attackers blow hits the target. Passing the target's health means we can also tell if we've killed the target. 
      - Key: entity that is attacking
      - Name: stonehearth:combat:melee\_attack\_connect
      - Args: action_details - some data about the nature of the attack (attacker, target, damage, target_health, target_exp). Target_exp is exp per hit.
   - **combat:battery** - triggered whenever a person is hit in combat. Triggered by _combat\_service.lua_: radiant.events.trigger_async(target, 'stonehearth:combat:battery', context)
      - Key: entity that was targeted
      - Name: stonehearth:combat:battery  
      - Args: context (some details on combat action included, like attacker, target, etc)

##Death

   - **kill_event** - triggered whenever a significant entity is killed and needs to inform its components etc that it is being destroyed. Only use if you are a component or an object related to that eneity, and must clean yourself up before the entity is destroyed. 
      - Key: entity that was killed
      - Name: stonehearth:kill_event
      - Args: none
      - Note: this is a synchronous event, so don't do anything crazy in the event handlers listening to it. On the flip side, you know the entity still exists. Please only use if you are a component on the entity and need to clean up after yourself. Otherwise, really complicated things can happen when a sync event goes off. 
   - **entity_killed** - triggered when an entity of significance (a villager, an enemy) dies due to in-game causes. Like kill_event but is async, and should be used in preference to kill_event UNLESS you are a component or object related to the entity that must clean yourself up on its removal from the game.
      - Key: radiant.entities 
      - Name: stonehearth:combat:entity_killed
      - Args: entity - the name, id, faction, and player_id of the entity that died
      - Note: this is async, so don't assume that the entity still exists. 


##Everyday Tasks
   
   - **sleep\_in\_bed** - triggered whenever we get up from sleeping in a bed, passes in entity that was sleeping
      - Key: entity that was sleeping
      - Name: stonehearth:sleep\_in\_bed
      - Args: none
   - **sleep\_on\_ground** - triggered when we get up from sleeping on the ground
      - Key: entity that was sleeping
      - Name: stonehearth:sleep\_on\_ground
      - Args: none
   - **level\_up** - triggered whenever a civilian levels up
      - Key: entity that is leveling up
      - Name: stonehearth:level_up
      - Args: level - level, job\_uri, job\_name

##Gathering

   - **clear\_trap** - triggered whenever a trapper clears a set trap, passes in id of trapped entity
      - Key: The trapper
      - Name: stonehearth:clear\_trap
      - Args: trapped\_entity\_id - the id of the entity that was trapped, nil if none
   - **befriend\_pet** - triggered whenever a trapper makes a pet friend
      - Key: The trapper
      - Name: stonehearth:befriend\_pet
      - Args: pet_id - the id of the pet, nil if none
   - **set\_trap** - triggered whenever a trapper sets a trap
      - Key: The trapper
      - Name: stonehearth:set\_trap
      - Args: none
   - **harvest\_crop** - triggered whenever a farmer harvests a plant from a field
      - Key: the farmer
      - Name: stonehearth:harvest\_crop
      - Args: crop - the crop that was harvested
   - **on\_renewable\_resource\_renewed** - triggered when a renewable resource renews itself
      - Key: the entity being renewed
      - Name: stonehearth:on\_renewable\_resource\_renewed
      - Args: target - the thing spawning the resource; available_resource - the type of resource that is now available
   - **on\_pasture\_animals\_changed** - triggered when the # of animals in the pasture changes
      - Key: the pasture
      - Name: stonehearth:on\_pasture\_animals\_changed
      - Args: none

##Crafting

   - **crafter:craft\_item** - triggered when the crafter finishes making an item
      - Key: the crafter
      - Name: stonehearth:crafter:craft\_item
      - Args: crafting data, containing recipe_data (the recipe json) and product (the item crafted, may be iconic)