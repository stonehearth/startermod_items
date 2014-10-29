# Overview 

Buffs (both positive and negative) are temporary changes to a character's attributes. They are applied in response to a situation or event. For example, sleeping on the ground may add the groggy buff to a character, causing them to be slower for a few hours after waking up. 

Buffs can be visible to the player (when it's appropriate to inform the player of the results of their choices) or they can be invisible, in which case they are representations of the world system. For example, the starving buff helps cue the player to the fact that his villagers are taking damage from hunger. In the meantime, the carry buff (which lowers speed when a person is carrying something) is a part of the world and is thus not displayed to the player. 

# Using Buffs

A buff is described in a .json file:

      {
         "type" : "debuff",
         "name" : "So Heavy!",
         "description" : "Carrying heavy things naturally causes people to move more slowly",
         "icon" : "file(carrying_buff.png)",
         "invisible_to_player" : true, //optional, false by default
         "modifiers" : {
            "speed" : {          //attribute modified by the buff
               "multiply" : 0.75 //can be add, multiply, min, or max
            }
         }
      } 

Is given a name in the manifest.json: 

      "buffs:carrying" : "file(data/buffs/carrying/carrying_buff.json)",

And is added/removed to a character via the following code: 

      radiant.entities.add_buff(self._entity, 'stonehearth:buffs:carrying')      
      radiant.entities.remove_buff(self._entity, 'stonehearth:buffs:carrying')

Buffs that are visible to the player appear in the unit frame of a selected character, in their character sheet, and associated with the attribute that the buff is manipulating. 

