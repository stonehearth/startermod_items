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
   self._sv.source = source
   self.__saved_variables:mark_changed()
end

function WaterfallComponent:set_target(target)
   self._sv.target = target
   self.__saved_variables:mark_changed()

   self:_trace_target()
end

function WaterfallComponent:set_height(height)
   if height < 0 then
      -- will get converted to a pressure channel soon
      height = 0
   end

   if height == self._sv.height then
      return
   end

   self._sv.height = height
   self._sv.region:modify(function(cursor)
         cursor:clear()
         cursor:add_cube(
            Cube3(
               -- note that the location of the waterfall entity is at the TOP of the region
               Point3(0, -height, 0),
               Point3(1, 0, 1)
            )
         )
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
   local water_level = target_water_component:get_water_level()
   local waterfall_location = radiant.entities.get_world_grid_location(self._entity)
   local height = waterfall_location.y - water_level
   self:set_height(height)
end

return WaterfallComponent
