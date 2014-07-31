local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local FarmerCrop =  require 'components.farmer_field.farmer_crop'

local PlantCrop = class()
PlantCrop.name = 'plant crop'
PlantCrop.does = 'stonehearth:plant_crop'
PlantCrop.status_text = 'planting crop'
PlantCrop.args = {
   target_crop = FarmerCrop,
}
PlantCrop.version = 2
PlantCrop.priority =  1

function PlantCrop:start_thinking(ai, entity, args)
   if ai.CURRENT.carrying == nil then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PlantCrop)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.target_crop:get_plantable_entity()})
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.target_crop:get_plantable_entity(),
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:run_effect', { effect = 'hoe', facing_entity = ai.ARGS.target_crop:get_field_spacer(ai.PREV.location) })
         :execute('stonehearth:call_method', {
                  obj = ai.ARGS.target_crop,
                  method = 'notify_planting_done',
                  args = {ai.BACK(2).location}
               })
