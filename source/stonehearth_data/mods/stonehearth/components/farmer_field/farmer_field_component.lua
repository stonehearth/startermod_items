--[[
   Stores data about the field in question
]]

local FarmerFieldComponent = class()
FarmerFieldComponent.__classname = 'FarmerFieldComponent'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

function FarmerFieldComponent:__init(entity, data_binding)
   self._entity = entity
   self._data = data_binding:get_data()

   self._data.size = {0, 0}
   self._data.location = nil
   self._data.contents = {}

   self._data_binding = data_binding
   self._data_binding:mark_changed()

   --TODO: listen on changes to faction, like stockpile?
end

function FarmerFieldComponent:extend(json)
   if json.size then
      self:set_size(json.size)
   end
end

---Call once when initializing a field
-- TODO: handle field resizing
-- size[1] is the x coordinate, size[2] is the y coordinate
--function FarmerFieldComponent:set_size(size)
--   self._data.size = { size[1], size[2] }
--end

function FarmerFieldComponent:init_contents(size, location, name, faction)
   self._data.size = { size[1], size[2] }
   self._data.location = location
   self._entity:get_component('unit_info'):set_display_name(name)
   self._entity:get_component('unit_info'):set_faction(faction)

   for x=1, self._data.size[1] do
      self._data.contents[x] = {}
      for y=1, self._data.size[2] do
         -- TODO: remove the default object; we just need it for visual effect for testing
         local field_spacer = radiant.entities.create_entity('stonehearth:default_object') 
         self._data.contents[x][y] = field_spacer
         local grid_location = Point3(location.x + x-1, 0, location.z + y-1)
         radiant.terrain.place_entity(field_spacer, grid_location)

         -- Tell the farmer scheduler to till this
         local town = stonehearth.town:get_town(faction)
         town:create_farmer_task('stonehearth:till_field', { field_spacer = field_spacer, field = self })
                                   :set_source(field_spacer)
                                   :set_name('till field task')
                                   :once()
                                   :start()
      end
   end

   --TODO: where to schedule tasks for planting and harvesting and destroying things?
end

--Should be the lower-left corner of the box min x and y coordinates
--Can get x and y by doing location.x/y
--function FarmerFieldComponent:set_location(location)
--   self._data.location = location
--end

--- Swap field spacer for a plot of dirt
--  TODO: model varients for the dirt
function FarmerFieldComponent:till_location(field_spacer)
   local grid_location = radiant.entities.get_world_grid_location(field_spacer)

   --local soil = radiant.entities.create_entity('stonehearth:default_object')       
   --radiant.terrain.place_entity(soil, grid_location)
end


return FarmerFieldComponent