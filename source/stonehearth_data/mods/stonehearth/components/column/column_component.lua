local build_util = require 'lib.build_util'
local constants = require('constants').construction

local Column = class()
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

-- called to initialize the component on creation and loading.
--
function Column:initialize(entity, json)
   self._entity = entity
   if not self._sv.connected_walls then
      self._sv.connected_walls = {}
      self.__saved_variables:mark_changed()
   end
end

function Column:set_brush(brush)
   self._sv.brush = brush
   self.__saved_variables:mark_changed()
   return self
end

-- make the rcs region match the shape of the column
--
function Column:layout()
   local height =  constants.STOREY_HEIGHT

   local rgn = self:_compute_column_shape()
   local origin = radiant.entities.get_world_grid_location(self._entity)
   rgn:translate(origin)

   self._entity:get_component('stonehearth:construction_data')
            :paint_on_world_region(self._sv.brush, rgn, true)

   return self
end

function Column:get_connected_walls(entity)
   return self._sv.connected_walls
end

function Column:get_accumulated_normal()
   local normal = Point3(0, 0, 0)
   for _, wall in pairs(self._sv.connected_walls) do
      local wall_normal = wall:get_component('stonehearth:wall')
                                 :get_normal()
      normal = normal + wall_normal
   end
   return normal
end

function Column:connect_to_roof(roof)
   assert(not self._sv.roof)
   self._sv.roof = roof
   self.__saved_variables:mark_changed()
end

function Column:get_roof(roof)
   return self._sv.roof
end

function Column:connect_to_wall(wall)
   self._sv.connected_walls[wall:get_id()] = wall
   self.__saved_variables:mark_changed()
end

function Column:clone_from(entity)
   return self
end

-- begin editing the column pointed to by `other_column`.  basically just
-- copy the shape and important variables in the save state
function Column:begin_editing(entity)
   self._editing_region = _radiant.client.alloc_region3()
   self._editing_region:modify(function(cursor)
         cursor:copy_region(self:_compute_column_shape())
      end)

   return self
end

-- return the 'editing region' for the column.  the editing region is a
-- region which fully covers the shape of the structure, minus all the
-- fixture and portals
--
function Column:get_editing_reserved_region()
   return self._editing_region
end

-- compute the shape of the column.
--
function Column:_compute_column_shape()
   -- xxx: this only works for trivial  1xnx1 brushes, sadly.

   local box = Cube3(Point3(0, 0, 0), Point3(1, constants.STOREY_HEIGHT, 1))

   -- if we're a client side authoring entity, we might not have a construction
   -- progress component.  that's ok!
   local cp = self._entity:get_component('stonehearth:construction_progress')
   if not cp then
      return Region3(box)
   end

   local roof = self._sv.roof   
   if roof then
      return build_util.grow_local_box_to_roof(roof, self._entity, box)
   end
   return Region3(box)
end


function Column:save_to_template()
   return {
      brush = self._sv.brush,
      roof = build_util.pack_entity(self._sv.roof),
      connected_walls = build_util.pack_entity_table(self._sv.connected_walls),
   }
end

function Column:load_from_template(data, options, entity_map)
   self._sv.brush = data.brush
   self._sv.roof  = build_util.unpack_entity(data.roof, entity_map)
   self._sv.connected_walls = build_util.unpack_entity_table(data.connected_walls, entity_map)
end

function Column:rotate_structure(degrees)
   build_util.rotate_structure(self._entity, degrees)
end

return Column
