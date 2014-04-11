local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local DirtPlotComponent =  require 'components.farmer_field.dirt_plot_component'

local PlantCrop = class()
PlantCrop.name = 'plant crop'
PlantCrop.does = 'stonehearth:plant_crop'
PlantCrop.args = {
   target_plot = Entity,
   dirt_plot_component = DirtPlotComponent,
   crop_type = 'string'
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
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.target_plot})
         :execute('stonehearth:run_effect', { effect = 'work', facing_entity = ai.ARGS.target_plot  })
         :execute('stonehearth:call_method', {
                  obj = ai.ARGS.dirt_plot_component,
                  method = 'plant_crop',
                  args = {ai.ARGS.crop_type}
               })
