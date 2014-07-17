local TraceCategories = _radiant.dm.TraceCategories
local Cube3 = _radiant.csg.Cube3
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local rng = _radiant.csg.get_default_rng()

local FarmerCrop = class()


function FarmerCrop:initialize(player_id, field, location, crop_type, auto_harvest, auto_plant, region, tilled_region)
   self._sv.player_id = player_id
   self._sv.field = field
   self._sv.crop_type = crop_type
   self._sv.auto_harvest = auto_harvest
   self._sv.auto_plant = auto_plant
   self._sv.location = location
   self._sv.total_region = Region3()
   self._sv.total_region:copy_region(region)
   
   self._sv.plantable_region_entity = radiant.entities.create_entity()
   radiant.terrain.place_entity(self._sv.plantable_region_entity, location)
   local d = self._sv.plantable_region_entity:add_component('destination')
   d:set_region(_radiant.sim.alloc_region())
     :set_auto_update_adjacent(true)

   self._sv.harvestable_region_entity = radiant.entities.create_entity()
   radiant.terrain.place_entity(self._sv.harvestable_region_entity, location)
   d = self._sv.harvestable_region_entity:add_component('destination')
   d:set_region(_radiant.sim.alloc_region())
     :set_auto_update_adjacent(true)
   
   self._sv.farm_tilled_region = tilled_region

   self.__saved_variables:mark_changed()   

   self:restore()
end

function FarmerCrop:restore()
   self._till_trace = self._sv.farm_tilled_region:trace('tilling trace', TraceCategories.SYNC_TRACE)
      :on_changed(function(region)
         self:_update_plantable_region()
      end)

   -- Always clear out the reserved region on load (entities will simply re-reserve
   -- when they run.)
   local d = self._sv.plantable_region_entity:get_component('destination')
   d:set_reserved(_radiant.sim.alloc_region())

   d = self._sv.harvestable_region_entity:get_component('destination')
   d:set_reserved(_radiant.sim.alloc_region())

   -- We have to re-listen to each dirt plot on load, to figure out when harvesting
   -- has been completed.
   for cube in self:get_harvestable_region():get():each_cube() do
      for pt in cube:each_point() do
         local plot = self:get_field_spacer(pt + self._sv.location)
         radiant.events.listen(plot, 'stonehearth:crop_removed', self, self.notify_harvest_done)
      end
   end

   self:_update_plantable_region()
   self:_create_planting_task()
end


function FarmerCrop:destroy()
   radiant.entities.destroy_entity(self._sv.plantable_region_entity)
   self._sv.plantable_region_entity = nil

   radiant.entities.destroy_entity(self._sv.harvestable_region_entity)
   self._sv.harvestable_region_entity = nil

   self._sv.farm_tilled_region = nil

   self.planting_task:destroy()
   self.planting_task = nil
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
   return self._sv.harvestable_region_entity:get_component('destination'):get_region()
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


function FarmerCrop:change_default_crop(new_crop)
   self._sv.crop_type = new_crop
   self.__saved_variables:mark_changed()
end


function FarmerCrop:_create_planting_task()
   self.planting_task = stonehearth.farming:plant_crop(self._sv.player_id, self)
end


function FarmerCrop:notify_planting_done(location)
   local x_offset = location.x - self._sv.location.x + 1
   local z_offset = location.z - self._sv.location.z + 1
   local dirt_plot = self._sv.field:get_contents()[x_offset][z_offset]
   local dirt_plot_comp = dirt_plot:get_component('stonehearth:dirt_plot')

   dirt_plot_comp:plant_crop(self._sv.crop_type)

   radiant.events.listen(dirt_plot, 'stonehearth:crop_removed', self, self.notify_harvest_done)

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
   
   self:get_plantable_region():modify(function(cursor)
      cursor:add_point(p)
   end)

   self:get_harvestable_region():modify(function(cursor)
      cursor:subtract_point(p)
   end)
   self.__saved_variables:mark_changed()

   return radiant.events.UNLISTEN
end

return FarmerCrop
