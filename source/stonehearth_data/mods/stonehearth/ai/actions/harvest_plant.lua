local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local HarvestPlant = class()

HarvestPlant.name = 'harvest plant'
HarvestPlant.does = 'stonehearth:harvest_plant'
HarvestPlant.args = {
   plant = Entity      -- the plant to chop
}
HarvestPlant.version = 2
HarvestPlant.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(HarvestPlant)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.plant })
         :execute('stonehearth:harvest_plant_adjacent', { plant = ai.ARGS.plant })
