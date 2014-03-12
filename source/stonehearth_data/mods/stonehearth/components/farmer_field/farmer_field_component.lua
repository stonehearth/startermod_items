--[[
   Stores data about the field in question
]]

local farming_service = stonehearth.farming

local FarmerFieldComponent = class()
FarmerFieldComponent.__classname = 'FarmerFieldComponent'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local rng = _radiant.csg.get_default_rng()

function FarmerFieldComponent:__create(entity, json)
   self._entity = entity

   if json.size then
      self:set_size(json.size)
   end

   self._data = {
      size = {0, 0}, 
      location = nil,
      contents = {},
      general_fertility = rng:get_int(1, 40)   --TODO; get from global service
   }

   self.__savestate = radiant.create_datastore(self._data)
   self.__savestate:mark_changed()

   --TODO: listen on changes to faction, like stockpile?
end

--TODO: Depending on how we eventually designate whether fields can overlap (no?)
--consider moving this into a central service
function FarmerFieldComponent:init_contents(size, location, name, faction)
   self._data.size = { size[1], size[2] }
   self._data.location = location
   self._entity:get_component('unit_info'):set_display_name(name)
   self._entity:get_component('unit_info'):set_faction(faction)

   for x=1, self._data.size[1] do
      self._data.contents[x] = {}
      for y=1, self._data.size[2] do
         local field_spacer = radiant.entities.create_entity('stonehearth:tilled_dirt') 
         local render_info = field_spacer:add_component('render_info')
         render_info:set_model_variant('untilled_ground')
         local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')
         dirt_plot_component:set_field(self._entity, location)

         self._data.contents[x][y] = field_spacer
         local grid_location = Point3(location.x + x-1, 0, location.z + y-1)
         radiant.terrain.place_entity(field_spacer, grid_location)

         -- Tell the farmer scheduler to till this
         -- TODO: store tasks somewhere so they can be cancelled
         local town = stonehearth.town:get_town(faction)
         town:create_farmer_task('stonehearth:till_field', { field_spacer = field_spacer, field = self })
                                   :set_source(field_spacer)
                                   :set_name('till field task')
                                   :once()
                                   :start()
      end
   end

   --TODO: where to schedule tasks for planting and harvesting and destroying things?
   self.__savestate:mark_changed()
end

--- Call when it's time to till the ground for the first time
--  TODO: model varients for the dirt
function FarmerFieldComponent:till_location(field_spacer)
   --TODO: get general fertility data from a global service
   --and maybe local fertility data too
   local local_fertility = rng:get_gaussian(self._data.general_fertility, 10)
   local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')

   --TODO: set moisture correctly
   dirt_plot_component:set_fertility_moisture(local_fertility, 50)
end


return FarmerFieldComponent