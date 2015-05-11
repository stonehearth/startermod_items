local TraceCategories = _radiant.dm.TraceCategories
local Cube3 = _radiant.csg.Cube3
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local rng = _radiant.csg.get_default_rng()

local FarmerCrop = class()

--[[
   This class manages the plantable/harvestable aspect of a crop in a field. 
   When a field is created, this crop manager is created as well, with the type of crop (including fallow)
   that will be planted in the field. 
   When a crop is first assigned, this manager creates the plant task for the farmers. 
   These tasks are ongoing. 
   Regions manage which parts of the field should be planted/harvested. The planting/harvesting tasks refer to these
   regions to figure out what they should be doing next. 
   When a field is left fallow, the farmers will till it under and the plantable region will be designated
   to be unplantable until the field is again assigned a crop. Then, the plantable region for all empty locations
   will be cleared allowing the plant actions to act on the field again. 
]]

function FarmerCrop:initialize(player_id, field, location, crop_type, auto_harvest, auto_plant, region, tilled_region)
   self._sv.player_id = player_id
   self._sv.field = field
   self._sv.crop_type = crop_type
   self._sv.auto_harvest = auto_harvest
   self._sv.auto_plant = auto_plant
   self._sv.location = location
   self._sv.total_region = Region3()
   self._sv.total_region:copy_region(region)
   self._sv.town = stonehearth.town:get_town(player_id)
   
   self._sv.plantable_region_entity = radiant.entities.create_entity('', { owner = self._entity })
   radiant.terrain.place_entity(self._sv.plantable_region_entity, location)
   local d = self._sv.plantable_region_entity:add_component('destination')
   d:set_region(_radiant.sim.alloc_region3())
     :set_auto_update_adjacent(true)

   self._sv.harvestable_region_entity = radiant.entities.create_entity('', { owner = self._entity })
   radiant.terrain.place_entity(self._sv.harvestable_region_entity, location)
   d = self._sv.harvestable_region_entity:add_component('destination')
   d:set_region(_radiant.sim.alloc_region3())
     :set_auto_update_adjacent(true)
   
   self._sv.farm_tilled_region = tilled_region

   self.__saved_variables:mark_changed()   

   self:_on_start_do_always()
   self:_create_planting_task()

end

function FarmerCrop:restore()
   self:_on_start_do_always()

   radiant.events.listen(radiant, 'radiant:game_loaded',
      function(e)
         self:_create_planting_task()
         return radiant.events.UNLISTEN
      end
   )
end

function FarmerCrop:_on_start_do_always()
   self._till_trace = self._sv.farm_tilled_region:trace('tilling trace', TraceCategories.SYNC_TRACE)
      :on_changed(function(region)
         self:_update_plantable_region()
      end)

   -- Always clear out the reserved region on load (entities will simply re-reserve
   -- when they run.)
   local d = self._sv.plantable_region_entity:get_component('destination')
   d:set_reserved(_radiant.sim.alloc_region3())

   d = self._sv.harvestable_region_entity:get_component('destination')
   d:set_reserved(_radiant.sim.alloc_region3())

   self._plot_listeners = {}
   -- We have to re-listen to each dirt plot on load, to figure out when harvesting
   -- has been completed.
   for pt in self:get_harvestable_region():get():each_point() do
      local plot = self:get_field_spacer(pt + self._sv.location)
      table.insert(self._plot_listeners, radiant.events.listen(plot, 'stonehearth:crop_removed', self, self.notify_harvest_done))
   end
   self:_update_plantable_region()
end


function FarmerCrop:destroy()
   for _, listener in ipairs(self._plot_listeners) do
      listener:destroy()
   end
   self._plot_listeners = nil

   if self._till_trace then
      self._till_trace:destroy()
      self._till_trace = nil
   end

   radiant.entities.destroy_entity(self._sv.plantable_region_entity)
   self._sv.plantable_region_entity = nil

   radiant.entities.destroy_entity(self._sv.harvestable_region_entity)
   self._sv.harvestable_region_entity = nil

   self._sv.farm_tilled_region = nil

   if self.planting_task then
      self.planting_task:destroy()
      self.planting_task = nil
   end
end


function FarmerCrop:get_field_spacer(location)
   local x_offset = location.x - self._sv.location.x + 1
   local z_offset = location.z - self._sv.location.z + 1
   return self._sv.field:get_contents()[x_offset][z_offset]
end


function FarmerCrop:get_total_region()
   return self._sv.total_region
end


function FarmerCrop:get_tilled_region()
   return self:get_total_region() - (self:get_total_region() - self._sv.farm_tilled_region:get())
end


function FarmerCrop:get_plantable_region()
   return self._sv.plantable_region_entity:get_component('destination'):get_region()
end


function FarmerCrop:get_plantable_entity()
   return self._sv.plantable_region_entity
end


function FarmerCrop:get_harvestable_region()
   local dc = self._sv.harvestable_region_entity:get_component('destination')
   if dc then
      return dc:get_region()
   end
   return nil
end


function FarmerCrop:get_harvestable_entity()
   return self._sv.harvestable_region_entity
end


function FarmerCrop:_update_plantable_region()
   self:get_plantable_region():modify(function(cursor)
      cursor:clear()
      cursor:add_region(self:get_tilled_region() - self:get_harvestable_region():get())
   end)
   self.__saved_variables:mark_changed()
end


function FarmerCrop:_update_harvestable_region()
   self:get_harvestable_region():modify(function(cursor)
      cursor:clear()
      cursor:add_region(self:get_tilled_region() - self:get_plantable_region():get())
   end)
   self.__saved_variables:mark_changed()
end

-- Change the default crop for this field but it will not take effect until
-- The new crop will not be planted unless a space is fallow (we won't raze existing crops)
-- If the field was previously fallow, then immediately clear all the empty spaces so 
-- new crop planting can happen. 
function FarmerCrop:change_default_crop(new_crop)
   local old_crop = self._sv.crop_type
   self._sv.crop_type = new_crop
   if not old_crop then
      --If the first crop was fallow, the planting task is never created. If 
      --we change to a new crop that is not fallow, and we have no task, create it now.
      if not self.planting_task then
         self:_create_planting_task()
      end
      self:_clear_field()

   end
   self.__saved_variables:mark_changed()
end

-- Tell the town that we should plant the crop
function FarmerCrop:_create_planting_task()
   if self._sv.crop_type and not self.planting_task then
      self.planting_task = self._sv.town:plant_crop(self)
   end
end


function FarmerCrop:notify_planting_done(location)
   local x_offset = location.x - self._sv.location.x + 1
   local z_offset = location.z - self._sv.location.z + 1
   local dirt_plot = self._sv.field:get_contents()[x_offset][z_offset]
   local dirt_plot_comp = dirt_plot:get_component('stonehearth:dirt_plot')

   dirt_plot_comp:plant_crop(self._sv.crop_type)

   radiant.events.listen_once(dirt_plot, 'stonehearth:crop_removed', self, self.notify_harvest_done)

   local p = Point3(x_offset - 1, 0, z_offset - 1)
   self:get_plantable_region():modify(function(cursor)
      cursor:subtract_point(p)
   end)

   self:get_harvestable_region():modify(function(cursor)
      cursor:add_point(p)
   end)
   self.__saved_variables:mark_changed()
end


-- This is called once per dirt-plot (we listen on each plot individually.)
function FarmerCrop:notify_harvest_done(e)
   local p = Point3(e.location.x - 1, 0, e.location.y - 1)
   self:_clear_location(p)

   self.__saved_variables:mark_changed()
end

function FarmerCrop:_clear_location(p)
   self:get_plantable_region():modify(function(cursor)
      cursor:add_point(p)
   end)

   self:get_harvestable_region():modify(function(cursor)
      cursor:subtract_point(p)
   end)
end

-- Look through all the areas of the field that do not have a crop in them 
-- and that are not furrows and clear that location's plantable region.
-- Useful for when we need to reset the planting region after a fallow crop.
function FarmerCrop:_clear_field()
   local field_x = #self._sv.field:get_contents()
   local field_z = #self._sv.field:get_contents()[1]

   for x=1, field_x do
      for z=1, field_z do
         local dirt_plot = self._sv.field:get_contents()[x][z]
         local dirt_plot_comp = dirt_plot:get_component('stonehearth:dirt_plot')
         if not dirt_plot_comp:get_contents() and not dirt_plot_comp:is_furrow() then
            local p = Point3(x - 1, 0, z - 1)
            self:_clear_location(p)
         end
      end
   end

end

return FarmerCrop
