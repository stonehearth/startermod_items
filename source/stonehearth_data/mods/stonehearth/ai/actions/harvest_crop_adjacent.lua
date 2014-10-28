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
   radiant.entities.turn_to_face(entity, args.crop)
   ai:execute('stonehearth:run_effect', { effect = 'fiddle' })

   -- it never hurts to be a little bit paranoid =)
   local crop_component = args.crop:get_component('stonehearth:crop')
   local carrying = radiant.entities.get_carrying(entity)
   if carrying and carrying:get_uri() ~= crop_component:get_product() then
      abort('not carrying the same type of crop')
   end

   if not radiant.entities.increment_carrying(entity, self:_get_num_to_increment(entity)) then
      local product_uri = crop_component:get_product()
      
      if product_uri ~= nil then
         local product = radiant.entities.create_entity(product_uri)
         local item_component = product:add_component('item')
         item_component:set_stacks(1)
         radiant.entities.pickup_item(entity, product)

         -- newly harvested drops go into your inventory immediately
         stonehearth.inventory:get_inventory(entity)
                                 :add_item(product)
      end
   end

   --Fire the event that describes the harvest
   radiant.events.trigger(entity, 'stonehearth:harvest_crop', {crop_uri = entity:get_uri()})

   ai:unprotect_entity(args.crop)
   radiant.entities.destroy_entity(args.crop)
end

--By default, we produce 1 item stack per basket
function HarvestCropAdjacent:_get_num_to_increment(entity)
   local num_to_spawn = 1

   --If the this entity has the right perk, spawn more than one
   local job_component = entity:get_component('stonehearth:job')
   if job_component and job_component:curr_job_has_perk('farmer_harvest_increase') then
      num_to_spawn = 2
   end

   return num_to_spawn
end

return HarvestCropAdjacent
