local Point2 = _radiant.csg.Point2

--[[
   Stores data about the field in question
]]

--local farming_service = stonehearth.farming

local FarmerFieldComponent = class()
FarmerFieldComponent.__classname = 'FarmerFieldComponent'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local rng = _radiant.csg.get_default_rng()

function FarmerFieldComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()   
   self.__saved_variables:set_controller(self)

   self._till_task = nil

   if not self._sv._initialized then
      -- creating for the 1st time...
      self._sv.crops = {}

      self._sv.soil_layer = radiant.entities.create_entity()
      self._sv.tilled_soil_region = _radiant.sim.alloc_region()

      self._sv._initialized = true
      self._sv.size = Point2(0, 0)
      self._sv.location = nil
      self._sv.contents = {}

      local min_fertility = stonehearth.constants.soil_fertility.MIN
      local max_fertility = stonehearth.constants.soil_fertility.MAX
      self._sv.general_fertility = rng:get_int(min_fertility, max_fertility)   --TODO; get from global service
      
      self._sv.crop_queue = {stonehearth.farming:get_crop_details('fallow')}
      self._sv.curr_crop = 1
      self._sv.auto_harvest = true
      self._sv.auto_replant = true

      self.destination = self._sv.soil_layer:add_component('destination')
      self.destination:set_region(_radiant.sim.alloc_region())
                       :set_reserved(_radiant.sim.alloc_region())
                       :set_auto_update_adjacent(true)
   else
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
         self.destination = self._sv.soil_layer:get_component('destination')
         self.destination:set_reserved(_radiant.sim.alloc_region()) -- xxx: clear the existing one from cpp land!
         if #self._sv.contents > 0 then
            self:_create_till_task()
         end
      end)
   end
   --self.__saved_variables:mark_changed()
   --TODO: listen on changes to faction, like stockpile?
end

function FarmerFieldComponent:_add_crop(region)
   return radiant.create_controller('stonehearth:farmer_crop',
      self._entity:get_component('unit_info'):get_player_id(),
      self:get_contents(),
      self._sv.location, 
      self:_get_next_queued_crop(),
      self._sv.auto_harvest,
      self._sv.auto_replant,
      region,
      self._sv.tilled_soil_region)
end

function FarmerFieldComponent:get_size()
   return self._sv.size
end

function FarmerFieldComponent:_get_bounds()
   local size = self:get_size()
   local bounds = Cube3(Point3(0, 0, 0), Point3(size.x, 1, size.y))
   return bounds
end

function FarmerFieldComponent:get_contents()
   return self._sv.contents
end

function FarmerFieldComponent:get_entity()
   return self._entity
end

function FarmerFieldComponent:get_soil_layer()
   return self._sv.soil_layer
end

function FarmerFieldComponent:get_soil_layer_region()
   return self._sv.soil_layer:get_component('destination'):get_region()
end

--- On destroy, remove all listeners from the plots
function FarmerFieldComponent:destroy()
   --Unlisten on all the field plot things
   for x=1, self._sv.size.x do
      for y=1, self._sv.size.y do
         local field_spacer = self._sv.contents[x][y].plot
         if field_spacer then
            local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')
            if dirt_plot_component then
               dirt_plot_component:set_field(nil, nil)
            end
         end
      end
   end
end


function FarmerFieldComponent:_create_till_task()
   local town = stonehearth.town:get_town(self:get_entity())
   self._till_task = town:create_task_for_group('stonehearth:task_group:simple_farming','stonehearth:till_entire_field', { field = self })
                             :set_source(self:get_soil_layer())
                             :set_name('till entire field task')
                             :set_priority(stonehearth.constants.priorities.farming.TILL)
                             :start()
end

--TODO: Depending on how we eventually designate whether fields can overlap (no?)
--consider moving this into a central service
function FarmerFieldComponent:create_dirt_plots(town, location, size)
   self._sv.size = Point2(size.x, size.y)
   self._sv.location = location

   radiant.terrain.place_entity(self:get_soil_layer(), location)

   for x=1, self._sv.size.x do
      self._sv.contents[x] = {}
      for y=1, self._sv.size.y do
         --init the dirt plot
         local field_spacer = self:_init_dirt_plot(location, x, y)
         self._sv.contents[x][y] = {}
         self._sv.contents[x][y].plot = field_spacer
      end
   end

   self:get_soil_layer_region():modify(function(cursor)
      cursor:clear()
      cursor:add_cube(self:_get_bounds())
   end)

   self:_create_till_task()

   self.__saved_variables:mark_changed()
end


function FarmerFieldComponent:get_field_spacer(location)
   local x_offset = location.x - self._sv.location.x + 1
   local z_offset = location.z - self._sv.location.z + 1
   return self._sv.contents[x_offset][z_offset].plot
end


--- Init the dirt plot's model variant, and place it in the world.
--  The dirt plot starts invisible, and then once tilled, appears.
function FarmerFieldComponent:_init_dirt_plot(location, x, y)
   local field_spacer = radiant.entities.create_entity('stonehearth:tilled_dirt') 
   local render_info = field_spacer:add_component('render_info')
   render_info:set_model_variant('untilled_ground')
   local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')
   dirt_plot_component:set_field(self._entity, {x=x, y=y})

   -- every even column in the farm is a furrow
   if x % 2 == 0 then
      dirt_plot_component:set_furrow(true)
   end

   local grid_location = Point3(location.x + x-1, 0, location.z + y-1)
   radiant.terrain.place_entity(field_spacer, grid_location)

   return field_spacer
end

--[[
--Fill ths in when we have a full queue implementation
--TODO: figure out the exact interaction between the UI and the queue update mechanism
function FarmerFieldComponent:update_queue(session, response, updated_queue, curr_crop_index)
   self._sv.crop_queue = updated_queue
   self._sv.curr_crop = curr_crop_index
   self.__saved_variables:mark_changed()
   return true
end

--]]

--Temporary: replaces queue item with a brand new item
function FarmerFieldComponent:change_default_crop(session, response, new_crop)
   self._sv.crop_queue = {stonehearth.farming:get_crop_details(new_crop)}

   -- Slight hack: check to make sure we we even have some crops to change; otherwise, create
   -- a new crop.
   if #self._sv.crops == 0 then
      local total_region = Region3()
      total_region:add_cube(self:_get_bounds())
      table.insert(self._sv.crops, self:_add_crop(total_region))
   else
      for crop in self._sv.crops do
         crop:change_default_crop(self:_get_next_queued_crop())
      end
   end
   self.__saved_variables:mark_changed()
   return true
end

--- Determine the crop that should be planted next, according the queue on this field
--  If the crop type is "fallow" the uri will be nil, and willr eturn nil
--  TODO: listen on an event to determine when to roll the crops over to the next period
function FarmerFieldComponent:_get_next_queued_crop()
   local next_plant = self._sv.crop_queue[self._sv.curr_crop]
   if next_plant then
      next_plant = next_plant.uri
   end
   return next_plant
end

--- Given the field and dirt data, harvest the crop
function FarmerFieldComponent:determine_auto_harvest(dirt_component)
   local do_harvest = self._sv.auto_harvest
   if dirt_component:get_player_override() then
      do_harvest = dirt_component:get_auto_harvest()
   end
   if do_harvest then
      local town = stonehearth.town:get_town(self._entity)
      local crop = dirt_component:get_contents()
      return town:harvest_crop(crop)
   end
end

function FarmerFieldComponent:notify_till_location_finished(location)
   local offset = location - radiant.entities.get_world_grid_location(self._entity)
   local field_spacer = self._sv.contents[offset.x + 1][offset.z + 1].plot
   local local_fertility = rng:get_gaussian(self._sv.general_fertility, stonehearth.constants.soil_fertility.VARIATION)
   local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')

   -- TODO: set moisture correctly
   -- This makes the plot visible!  Umm....
   dirt_plot_component:set_fertility_moisture(local_fertility, 50)

   self:get_soil_layer_region():modify(function(cursor)
      cursor:subtract_point(offset)
   end)

   if not dirt_plot_component:is_furrow() then
      self._sv.tilled_soil_region:modify(function(cursor)
         cursor:add_point(offset)
      end)
   end
   self.__saved_variables:mark_changed()
end

return FarmerFieldComponent