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
