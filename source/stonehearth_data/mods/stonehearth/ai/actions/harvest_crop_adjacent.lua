local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

local HarvestCropAdjacent = class()
HarvestCropAdjacent.name = 'harvest crop adjacent'
HarvestCropAdjacent.does = 'stonehearth:harvest_crop_adjacent'
HarvestCropAdjacent.args = {
   crop = Entity      -- the crop entity to harvest
}
HarvestCropAdjacent.version = 2
HarvestCropAdjacent.priority = 1


function HarvestCropAdjacent:run(ai, entity, args)
   local crop = args.crop
   local data = radiant.entities.get_entity_data(args.crop, 'stonehearth:crop')

   radiant.entities.turn_to_face(entity, crop)
   ai:execute('stonehearth:run_effect', { effect = data.harvest_effect })

   radiant.entities.destroy_entity(crop)

   if not radiant.entities.increment_carrying(entity) then
      local product = radiant.entities.create_entity(data.product)
      local item_component = product:add_component('item')
      item_component:set_stacks(1)
      radiant.entities.pickup_item(entity, product)
   end
end

return HarvestCropAdjacent
