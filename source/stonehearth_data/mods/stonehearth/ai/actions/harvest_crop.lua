local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local HarvestCropAction = class()

HarvestCropAction.name = 'harvest crop'
HarvestCropAction.does = 'stonehearth:harvest_crop'
HarvestCropAction.args = {
   crop = Entity      -- the crop entity to harvest
}
HarvestCropAction.version = 2
HarvestCropAction.priority = 1

function HarvestCropAction:start_thinking(ai, entity, args) 
   local carrying = ai.CURRENT.carrying

   -- if we're not carrying anything, proceed
   if not carrying then
      ai.set_think_output()
      return
   end

   -- at this point we're definitely carring something

   -- if the thing we're carrying isn't the container for this crop, 
   -- we cannot proceed
   local data = radiant.entities.get_entity_data(args.crop, 'stonehearth:crop')
   if not data then
      return
   end

   if carrying:get_uri() ~= data.product then
      return
   end


   local item_component = carrying:add_component('item')
   if item_component:get_stacks() >= item_component:get_max_stacks() then
      return
   end
   ai.set_think_output()   
end

local ai = stonehearth.ai
return ai:create_compound_action(HarvestCropAction)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.crop })
         :execute('stonehearth:harvest_crop_adjacent', { crop = ai.ARGS.crop })
         :execute('stonehearth:trigger_event', {
            source = stonehearth.personality,
            event_name = 'stonehearth:journal_event',
            event_args = {
               entity = ai.ENTITY,
               description = 'harvest_entity',
            },
         })
