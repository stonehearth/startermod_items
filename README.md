# startermod_items
a sample mod showing how to create different types of items

This mod assumes that you have downloaded and done the exercises in https://github.com/stonehearth/startermod_basic

- use stonehearth debugtools
- use debugtools's clone stamper to put items down in the game
- add an item to the carpenter's recipe list, so it can be crafted.

Startermod_items contains various items that you can add to stonehearth. Make tweaks to the .json, change the models, or use these items as templates to create your own items!

Observe that we have: 
- sample_melee_weapon
- sample_armor
- sample_shield

- sample buff

- sample_lamp (by Relyss)
- hung_lamp (by Relyss)
- sample_fountain (by Relyss)
- sample_fountain:fine (by Relyss)

To see/use these items in the game, follow the instructions in startermod_basic for adding items to the world via the clone stamp tool, or by adding them to a crafter's recipe list. 

Notes by Item Type: 

sample_melee_weapon
- be sure to add the iconic version to the world
- iconic version will be automatically equipped by a footman, if there is a footman in the world
- to edit sword stats, edit legendary_sword.json
- to edit the sword model, edit legendary_sword.qmo, and export as legendary_sword.qb
- the sword will also grant the sample_buff mentioned above, which is added via the "injected_buffs" line in the equipment_piece component. 

sample_armor
- like the sword, be sure to add the iconic version to the world
- the iconic version will automatically be equipped by a footman
- to edit stats/model, see sample_melee_weapon, above.

sample_shield
- similar to sample sword and armor, footmen will automatically equip iconic version
- to edit shield, be sure to edit both the iconic and the equipped versions of the .qmo files. The shield is carried sideways by the footman. 

sample_lamp
- implements the lamp component, which causes an effect to run at sundown, and to stop running at sun-up.
- in this case, the effect assigned to "light_effect" is the "sample_lamp_effect", which is stored in this mod's data\effects file
- sample_lamp_effect re-uses an existing particle effect (called cubemitter) from the regular stonehearth mod, called "particles/lamp/lamp.cubemitter". It makes sure that the cubes of light are being emitted from the correct location on the model. 

hung_lamp
- is just like sample_lamp except that comparing hung_lamppost_effect to sample_lamp_effect illustrates how to move the effect elsewhere on the model. 

sample_fountain
- illustrates how to create brand-new cubemitters for the water effects. The effects are added via the effect_list component, which will play anything given to it immediately on placement in the world, and are defined in data/effects/sample_fount_effect
- the cubemitters mentioned in the effect can be found in data/horde/particles/water
- cubemitters are complicated, but feel free to mess with the values in the file and see what happens!

sample_fountain_fine
- just like sample_fountain, but nicer, in case you'd like to include a "fine" version for a crafter to create at random