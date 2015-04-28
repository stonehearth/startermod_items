local csg_lib = require 'lib.csg.csg_lib'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

local WaterfallComponent = class()

function WaterfallComponent:__init()
end

function WaterfallComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv.region = _radiant.sim.alloc_region3()
      self._sv.volume = 0
      self._sv._initialized = true
      self.__saved_variables:mark_changed()
   else
      self:_restore()
   end
end

function WaterfallComponent:_restore()
   self:_trace_target()
end

function WaterfallComponent:destroy()
   self:_destroy_target_trace()
end

function WaterfallComponent:set_source(source)
   if self._sv.source == source then
      return
   end

   self._sv.source = source
   self.__saved_variables:mark_changed()
end

function WaterfallComponent:set_target(target)
   if self._sv.target == target then
      return
   end

   self._sv.target = target
   self.__saved_variables:mark_changed()

   self:_trace_target()
end

-- world coordinates
function WaterfallComponent:set_top_elevation(top_elevation)
   if self._sv.top_elevation == top_elevation then
      return
   end

   self._sv.top_elevation = top_elevation
   self:_update_region()
   self.__saved_variables:mark_changed()
end

-- world coordinates
function WaterfallComponent:set_bottom_elevation(bottom_elevation)
   if self._sv.bottom_elevation == bottom_elevation then
      return
   end

   self._sv.bottom_elevation = bottom_elevation
   self:_update_region()
   self.__saved_variables:mark_changed()
end

function WaterfallComponent:_update_region()
   local cube = nil

   if self._sv.top_elevation and self._sv.bottom_elevation then
      local location = radiant.entities.get_world_grid_location(self._entity)
      local top = self._sv.top_elevation - location.y
      local bottom = self._sv.bottom_elevation - location.y

      if bottom > top then
         bottom = top
      end

      cube = Cube3(Point3.zero)
      cube.max.y = top
      cube.min.y = bottom
   end

   self._sv.region:modify(function(cursor)
         cursor:clear()

         if cube then
            cursor:add_cube(cube)
         end
      end)
   self.__saved_variables:mark_changed()
end

function WaterfallComponent:set_volume(volume)
   self._sv.volume = volume
   self.__saved_variables:mark_changed()
end

function WaterfallComponent:get_region()
   return self._sv.region:get()
end

function WaterfallComponent:_trace_target()
   self:_destroy_target_trace()

   local target = self._sv.target
   if not target then
      return
   end

   local target_water_component = target:add_component('stonehearth:water')
   self._target_trace = target_water_component.__saved_variables:trace_data('waterfall component')
      :on_changed(function()
               self:_update()
            end
         )
      :push_object_state()
end

function WaterfallComponent:_destroy_target_trace()
   if self._target_trace then
      self._target_trace:destroy()
      self._target_trace = nil
   end
end

function WaterfallComponent:_update()
   local target = self._sv.target
   if not target then
      return
   end

   local target_water_component = target:add_component('stonehearth:water')
   local target_water_level = target_water_component:get_water_level()
   self:set_bottom_elevation(target_water_level)
end

return WaterfallComponent
