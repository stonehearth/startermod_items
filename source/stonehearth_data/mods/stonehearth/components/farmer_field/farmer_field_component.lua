--[[
   Stores data about the field in question
]]

local FarmerFieldComponent = class()
FarmerFieldComponent.__classname = 'FarmerFieldComponent'

local Cube3 = _radiant.csg.Cube3
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('timer')

function FarmerFieldComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()   

   if not self._sv._initialized then
      -- creating for the 1st time...
      self._sv.crops = {}

      self._sv.soil_layer = radiant.entities.create_entity('stonehearth:farmer:field_layer:tillable', { owner = self._entity })
      self._sv.tilled_soil_region = _radiant.sim.alloc_region3()

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

      self:_create_harvestable_layer()

      self._sv.soil_layer:add_component('destination')
                           :set_region(_radiant.sim.alloc_region3())
                           :set_reserved(_radiant.sim.alloc_region3())
                           :set_auto_update_adjacent(true)

      self._sv.soil_layer:add_component('stonehearth:farmer_field_layer')
                           :set_farmer_field(self)

   else
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
         self._sv.soil_layer:get_component('destination')
                              :set_reserved(_radiant.sim.alloc_region3()) -- xxx: clear the existing one from cpp land!

         self._sv.harvestable_layer:get_component('destination')
                              :set_reserved(_radiant.sim.alloc_region3()) -- xxx: clear the existing one from cpp land!
      end)
   end
   --self.__saved_variables:mark_changed()
   --TODO: listen on changes to faction, like stockpile?
end

function FarmerFieldComponent:_create_harvestable_layer()
   self._sv.harvestable_layer = radiant.entities.create_entity('stonehearth:farmer:field_layer:harvestable', { owner = self._entity })
   self._sv.harvestable_layer:add_component('destination')
                              :set_region(_radiant.sim.alloc_region3())
                              :set_reserved(_radiant.sim.alloc_region3())
                              :set_auto_update_adjacent(true)

   self._sv.harvestable_layer:add_component('stonehearth:farmer_field_layer')
                                 :set_farmer_field(self)
end

function FarmerFieldComponent:_add_crop(region)
   return radiant.create_controller('stonehearth:farmer_crop',
      self._entity:get_component('unit_info'):get_player_id(),
      self,
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
   return self._sv.soil_layer:get_component('destination')
                                :get_region()
end

function FarmerFieldComponent:get_harvestable_layer()
   return self._sv.harvestable_layer
end

function FarmerFieldComponent:get_harvestable_layer_region()
   return self._sv.harvestable_layer:get_component('destination')
                                       :get_region()
end

--- On destroy, remove all listeners from the plots
function FarmerFieldComponent:destroy()
   for i, crop in ipairs(self._sv.crops) do
      crop:destroy()
      self._sv.crops[i] = nil
   end

   --Unlisten on all the field plot things
   for x=1, self._sv.size.x do
      for y=1, self._sv.size.y do
         local field_spacer = self._sv.contents[x][y]
         if field_spacer then
            -- destroys the dirt and crop entities
            -- if you don't want them to disappear immediately, then we need to figure out how they get removed from the world
            -- i.e. render the plant as decayed and implement a work task to clear rubble
            -- remember to undo ghost mode if you keep the entities around (see stockpile_renderer:destroy)
            radiant.entities.destroy_entity(field_spacer)
            self._sv.contents[x][y] = nil
         end
      end
   end

   radiant.entities.destroy_entity(self._sv.soil_layer)
   self._sv.soil_layer = nil

   radiant.entities.destroy_entity(self._sv.harvestable_layer)
   self._sv.harvestable_layer = nil
end

--TODO: Depending on how we eventually designate whether fields can overlap (no?)
--consider moving this into a central service
function FarmerFieldComponent:create_dirt_plots(town, location, size)
   self._sv.size = Point2(size.x, size.y)
   self._sv.location = location

   radiant.terrain.place_entity(self:get_soil_layer(), location)
   radiant.terrain.place_entity(self:get_harvestable_layer(), location)

   for x=1, self._sv.size.x do
      table.insert(self._sv.contents, {})
   end

   self:get_soil_layer_region():modify(function(cursor)
      cursor:clear()
      cursor:add_cube(self:_get_bounds())
   end)
   radiant.events.trigger_async(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', self._sv.soil_layer)

   self:notify_score_changed()
   
   self.__saved_variables:mark_changed()
end


function FarmerFieldComponent:get_field_spacer(location)
   local x_offset = location.x - self._sv.location.x + 1
   local z_offset = location.z - self._sv.location.z + 1
   return self._sv.contents[x_offset][z_offset]
end


--- Init the dirt plot's model variant, and place it in the world.
--  The dirt plot starts invisible, and then once tilled, appears.
function FarmerFieldComponent:_init_dirt_plot(location, x, y)
   local field_spacer = radiant.entities.create_entity('stonehearth:tilled_dirt', { owner = self._entity }) 
   local render_info = field_spacer:add_component('render_info')
   render_info:set_model_variant('untilled_ground')
   local dirt_plot_component = field_spacer:add_component('stonehearth:dirt_plot')
   dirt_plot_component:set_field(self._entity, Point2(x, y))

   -- every even column in the farm is a furrow
   if x % 2 == 0 then
      dirt_plot_component:set_furrow(true)
   end

   radiant.terrain.place_entity_at_exact_location(field_spacer, location)

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
      for _, crop in pairs(self._sv.crops) do
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
      self:get_harvestable_layer_region():modify(function(cursor)
         local l = dirt_component:get_location()
         cursor:add_point(Point3(l.x - 1, 0, l.y - 1))
      end)
      radiant.events.trigger_async(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', self._sv.harvestable_layer)
   end
end

function FarmerFieldComponent:harvest_crop(crop)
   self:get_harvestable_layer_region():modify(function(cursor)
      local d = crop:get_component('stonehearth:crop'):get_dirt_plot()
      local l = d:get_component('stonehearth:dirt_plot'):get_location()
      cursor:subtract_point(Point3(l.x - 1, 0, l.y - 1))
   end)
   radiant.events.trigger_async(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', self._sv.harvestable_layer)   
end

function FarmerFieldComponent:crop_at(location)
   local field_spacer = self:get_field_spacer(location)
   local dirt_plot = field_spacer:get_component('stonehearth:dirt_plot')
   return dirt_plot:get_contents()
end

function FarmerFieldComponent:notify_till_location_finished(location)
   local offset = location - radiant.entities.get_world_grid_location(self._entity)
   local field_spacer = self:_init_dirt_plot(location, offset.x + 1, offset.z + 1)
   self._sv.contents[offset.x + 1][offset.z + 1] = field_spacer
   local local_fertility = rng:get_gaussian(self._sv.general_fertility, stonehearth.constants.soil_fertility.VARIATION)
   local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')

   -- TODO: set moisture correctly
   -- This makes the plot visible!  Umm....
   dirt_plot_component:set_fertility_moisture(local_fertility, 50)

   self:get_soil_layer_region():modify(function(cursor)
      cursor:subtract_point(offset)
   end)
   radiant.events.trigger_async(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', self._sv.soil_layer)

   if not dirt_plot_component:is_furrow() then
      self._sv.tilled_soil_region:modify(function(cursor)
         cursor:add_point(offset)
      end)
   end

   self.__saved_variables:mark_changed()
end

function FarmerFieldComponent:notify_score_changed()
   if not self._score_dirty then
      self._score_dirty = true
      radiant.events.listen_once(radiant, 'stonehearth:gameloop', self, self._update_score)
   end
end

function FarmerFieldComponent:_update_score()
   local field_size = self:get_size()
   local field_contents = self:get_contents()
   local score = 0

   self._score_dirty = false

   for x=1, field_size.x do
      for y=1, field_size.y do
         local field_spacer = field_contents[x][y]
         if field_spacer then
            local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')
            if dirt_plot_component then
               --add a score based on the quality of the dirt
               local fertility, moisture = dirt_plot_component:get_fertility_moisture()
               score = score + fertility / 10

               --add a score for the plant if there's a plant in the dirt
               local crop = dirt_plot_component:get_contents()
               if crop then
                  --TODO: change this value based on a score in the crop's json
                  score = score + 1
               end
            end
         end
      end
   end
   score = radiant.math.round(score)
   stonehearth.score:change_score(self._entity, 'net_worth', 'field component', score / 10)
end

return FarmerFieldComponent