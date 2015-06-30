local Point3 = _radiant.csg.Point3

local DirtPlotComponent = class()

function DirtPlotComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.fertility = 0
      self._sv.moisture = 0
      self._sv.fertility_category = nil
      self._sv.parent_field = nil
      self._sv.field_location = nil
      self._sv.contents = nil
      self._sv.last_planted_type = nil
      self._sv.player_override = false
      self._sv.auto_replant = nil
      self._sv.auto_harvest = nil
      self._sv.is_furrow = nil
   else
      radiant.events.listen(radiant, 'radiant:game_loaded',
         function(e)
            self:_listen_to_crop_events()
            return radiant.events.UNLISTEN
         end
      )
   end

end

function DirtPlotComponent:destroy()
   self:_unlisten_from_crop_events()

   if self._sv.contents then
      radiant.entities.destroy_entity(self._sv.contents)
      self._sv.contents = nil
   end
end

--- Set the field and location in field for this plot
--  @param field: the field entity that we care about
--  @param location: the x/y coordinates of the plot. location.x and location.y are the coordinates.
function DirtPlotComponent:set_field(parent_field, location)
   self._sv.parent_field = parent_field
   self._sv.field_location = location
   self.__saved_variables:mark_changed()
end

function DirtPlotComponent:get_field()
   return self._sv.parent_field
end

--- Returns the x,y coordinates in the field that this plot is at
--  @returns location.x and location.y
function DirtPlotComponent:get_location()
   return self._sv.field_location
end

--- Even columns of the farm field are furrows, which are not plantable
-- @returns true if this cell of the farm is a furrow
function DirtPlotComponent:is_furrow()
   return self._sv.is_furrow
end

--- Set whether this cell of the farm is a furrow or now. Plants do not grow in furrows
-- @param value: set true to make this dirt plot a furrow
function DirtPlotComponent:set_furrow(value)
   self._sv.is_furrow = value
   self.__saved_variables:mark_changed()
end

--- Given data about the plot, set the right state variables
--  @param fertility: a number between 0 and 40
--  @param moisture: a number between 0 and 100
function DirtPlotComponent:set_fertility_moisture(fertility, moisture)
   self._sv.fertility = fertility
   self._sv.moisture = moisture
   self:_update_visible_soil_state()
   self:_notify_score_changed()
   self.__saved_variables:mark_changed()
end

function DirtPlotComponent:get_fertility_moisture()
   return self._sv.fertility,  self._sv.moisture 
end

function DirtPlotComponent:get_contents()
   return self._sv.contents
end

function DirtPlotComponent:get_last_planted_type()
   return self._sv.last_planted_type
end

--- Did the player override the field policy for this plot? 
--  If so set the player override and the auto-replant/harvest for this plot
function DirtPlotComponent:set_player_override(player_override, do_replant, do_harvest)
   self._sv.player_override = player_override
   self._sv.auto_replant = do_replant
   self._sv.auto_harvest = do_harvest
   self.__saved_variables:mark_changed()
end

function DirtPlotComponent:get_player_override()
   return self._sv.player_override
end

function DirtPlotComponent:get_replant()
   return self._sv.auto_replant
end

function DirtPlotComponent:get_auto_harvest()
   return self._sv.auto_harvest
end

--TODO: incorporate the moisture of the soil
function DirtPlotComponent:_update_visible_soil_state()
   local fertility_category = nil
   if self._sv.is_furrow then
      fertility_category = 'furrow'
   elseif  self._sv.fertility < stonehearth.constants.soil_fertility.POOR then
      fertility_category = 'dirt_1'
   elseif  self._sv.fertility < stonehearth.constants.soil_fertility.FAIR then
      fertility_category = 'dirt_2'
   elseif  self._sv.fertility < stonehearth.constants.soil_fertility.GOOD then
      fertility_category = 'dirt_3'
   else 
      fertility_category = 'dirt_4'
   end

   --If the category has changed, propagate the change through all
   --visible elements
   if fertility_category ~= self._sv.fertility_category then
      self._sv.fertility_category = fertility_category

      local render_info = self._entity:add_component('render_info')
      render_info:set_model_variant(fertility_category)
      local dirt_name, dirt_description = stonehearth.farming:get_dirt_descriptions(fertility_category)
      radiant.entities.set_name(self._entity, dirt_name)
      radiant.entities.set_description(self._entity, dirt_description)
   end
end

--- Note: Plots are assumed to start empty. If you programatically put a plant down
--- in a crop, be sure to start with this function. 

--- Called when something is planted on me
--  @param e.crop = new crop planted
function DirtPlotComponent:plant_crop(crop_type)
   --Assert that there's nothing here 
   --TODO: use tasks to make sure things aren't planted twice
   --assert(self._sv.contents == nil, "error, trying to plant on an occupied square")
   if self._sv.contents ~= nil or not crop_type then
      return
   end

   local planted_entity = radiant.entities.create_entity(crop_type, { owner = self._entity })
   radiant.terrain.place_entity(planted_entity, radiant.entities.get_world_grid_location(self._entity))
   self._sv.contents = planted_entity
   self._sv.last_planted_type = crop_type

   --If the planted entity is a crop, add a reference to the dirt it sits on. 
   local crop_component = planted_entity:get_component('stonehearth:crop')
   crop_component:set_dirt_plot(self._entity)

   self:_listen_to_crop_events()
   self:_notify_score_changed()

   self.__saved_variables:mark_changed()
end

--When a crop is on us, do these things:
function DirtPlotComponent:_listen_to_crop_events()
   local contents = self._sv.contents

   --listen for if the planted crop gets destroyed for any reason
   if not self._listening_to_crop_events and contents and contents:is_valid() then
      self._removed_listener = radiant.events.listen(contents, 'radiant:entity:pre_destroy', self, self._on_crop_removed)
      self._harvestable_listener = radiant.events.listen(contents, 'stonehearth:crop_harvestable', self, self._on_crop_harvestable)
      self._listening_to_crop_events = true
   end
end

function DirtPlotComponent:_unlisten_from_crop_events()
   local contents = self._sv.contents

   if self._listening_to_crop_events and contents and contents:is_valid() then
      self._removed_listener:destroy()
      self._removed_listener = nil

      self._harvestable_listener:destroy()
      self._harvestable_listener = nil
      self._listening_to_crop_events = false
   end
end

--- If the crop is now harvestable, let the field know, so it can handle it according to policy
function DirtPlotComponent:_on_crop_harvestable()
   local crop = self._sv.contents
   local field = self._sv.parent_field
   if field and field:is_valid() and crop:is_valid() then
      field:get_component('stonehearth:farmer_field')
           :determine_auto_harvest(self)
   end
end

--- Called when something is removed from me
function DirtPlotComponent:_on_crop_removed()
   --Assert that there's something here 
   --TODO: use tasks to only allow things to be planted/harvested once
   --assert(self._sv.contents ~= nil, "error, removing a crop that isn't there")
   if self._sv.contents == nil then
      return
   end

   self:_unlisten_from_crop_events()
   self._sv.contents = nil

   --Tell the listening field that we're empty, so it can decide what to do next
   radiant.events.trigger_async(self._entity, 'stonehearth:crop_removed', {
      plot_entity = self._entity,
      location = self._sv.field_location,
   })

   self:_notify_score_changed()
end

function DirtPlotComponent:_notify_score_changed()
   if self._sv.parent_field then
      self._sv.parent_field:get_component('stonehearth:farmer_field')
                              :notify_score_changed()
   end
end

return DirtPlotComponent
