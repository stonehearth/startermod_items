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
      self._sv._initialized = true
      self.__saved_variables:mark_changed()
   else
      self:_restore()
   end
end

function WaterfallComponent:_restore()
end

function WaterfallComponent:destroy()
end

function WaterfallComponent:set_source_entity(source_entity)
   self._sv.source_entity = source_entity
   self.__saved_variables:mark_changed()
end

function WaterfallComponent:set_drain_entity(drain_entity)
   self._sv.drain_entity = drain_entity
   self.__saved_variables:mark_changed()
end

function WaterfallComponent:set_height(height)
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

function WaterfallComponent:get_region()
   return self._sv.region:get()
end

return WaterfallComponent
