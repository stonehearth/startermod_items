#Overview

All interactable objects in Stonehearth (people, rabbits, chairs, houses, etc) are entities, and entities are described by their components. 

Components describe things like its backpack, its stats, etc. If  game logic of any sort is associated with an entity, there is likely a component attached to that entity covering that game logic.

#Adding a Component

## entity_name.txt
An entity is represented by a text file containing json describing that entity. 

The file describes which components should be added to the entity. The entity may inherit other components if it has mixins, etc.

Here's a simple example from arch_backed_chair.json: 

      {
         "mixins" : "file(arch_backed_chair_ghost.json)",

         "components": {
            "stonehearth:entity_forms" : {
               "iconic_form" : "file(arch_backed_chair_iconic.json)",
               "ghost_form" : "file(arch_backed_chair_ghost.json)",
               "placeable_on_ground" : true
            },
            "region_collision_shape" : {
               "region": [
                  {
                     "min" : { "x" : 0, "y" : 0, "z" : 0 },
                     "max" : { "x" : 1, "y" : 2, "z" : 1 }
                  }
               ]
            }
         },
         "entity_data" : {
            "stonehearth:chair" : {}
         }
      }

In this case, both 'stonehearth:entity\_forms' and 'region\_collision\_shape' are components that will be added to the entity when it is created. Others will be added based on whatever is in the mixin, arch\_back_chair\_ghost.json. 

## From Lua
You can also add a component by calling the add\_component function on the entity from lua. Either form works: 

      entity_variable:add_component('name_of_component')
      radiant.entities.add_component(entity_variable, 'name_of_component')

If the component is not found (for example, because the player has turned it off in the settings screen) the component will return nil.

The component's name will change based on it's mod, and whether it's implemented in C++ or Lua.

# Making a Component

C++ components are rare, and close to the heart of the game, and hard to change. The C++ implementation and functions to get/set data on these components is available from stonehearth\source\_simulation\script\lua_om.cpp

Most components are implemented in Lua, and if you are writing a mod, your component should be written in Lua also. 

Lua components look like this: 

      local MyComponent = class()

      -- The component's constructor, called on load
      -- entity is usually the entity that the component is created on
      -- json is the associated component data from the entity's .json file
      function MyComponent:initialize(entity, json)
         self._entity = entity
         self._sv = self.__saved_variables:get_data()
         -- setup other constructor stuff here
         -- One common pattern is: 
         if not self._sv._initialized then
            --first time init stuff
         else
            --code that only runs on load
         end
      end
        
      function MyComponent:getter_or_setter()
      end

      function MyComponent:other_functions()
      end

      return MyComponent

self in the component refers to the component object itself.
All components are initalized with a self.\_\_saved\_variables object that we can use to stash data that must be persisted between one save/load and another. 

If the component is initialized from the .json file of an entity, then the init function's json variable recieves any json passed into the component via the entity's .json file. For example, if this is the .json file for a carpenter's workshop, and the component in question is the crafter's workshop componnet, then it {foo:bar} will be passed in the json varaible in the initialize function above: 

      {
         "type": "entity",
         "components" : {
            "unit_info" : {
               "name" :  "Carpenter Workbench",
               "description" : "Clean enough."
            },
            "stonehearth_crafter:workshop" : {
               "foo": "bar"
            }
         }
      }

Finally, to access the component, it must be referenced in a module's manifest.json file. For example, a sample mod named crafter_workshop:

      {
         "info" :{
            "name" : "Stonehearth"
         },
         "components" :  {
            "workshop" : "file(components/workshop_component.lua)",
            "crafter"  : "file(components/crafter_component.lua)"
         }
      }

The component's name would then be: 'crafter_workshop:workshop'

## Notes on saving/loading/using in the UI

Calling mark_changed on the saved_variables object causes the component to update itself to anyone who is listening on changes to the component: 

      self.__saved_variables:mark_changed() 

Also, saved variables with leading _'s are saved, but not remoted. Keep our bookeeping data in private variables, but make a public ones for remoting to the ui, which just contains the effectivevalue of the attribute.

TODO: write a section on accessing component data from the UI

# Using a Component
Once you have a reference to an entity, usually generated by the creation of the entity, you can then access its components through:

      local comp = entity:get_component('name of component')

You can then perform any actions on comp that are implemented on that object. For example:

      local foo = comp:get_foo()
      comp:my_function_1(bar)