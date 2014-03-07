local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local PlantCrop = class()
PlantCrop.name = 'plant crop'
PlantCrop.does = 'stonehearth:plant_crop'
PlantCrop.args = {
   target_plot = Entity,
   crop_type = 'string', 
   location = Point3
}
PlantCrop.version = 2
PlantCrop.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(PlantCrop)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.target_plot})
         --TODO: If there is already a plant there, and it is harvestable, harvest it
         --TODO: if it is not harvestable, then plow it under 
         :execute('stonehearth:run_effect', { effect = 'work', facing_entity = ai.ARGS.target_plot  })
         :execute('stonehearth:create_entity', {entity_type = ai.ARGS.crop_type, location = ai.ARGS.location})
