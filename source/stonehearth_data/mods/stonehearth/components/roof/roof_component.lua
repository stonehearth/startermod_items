local csg_lib = require 'lib.csg.csg_lib'
local build_util = require 'lib.build_util'
local constants = require('constants').construction

local Roof = class()
local Point2 = _radiant.csg.Point2
local Region2 = _radiant.csg.Region2
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local NineGridBrush = _radiant.voxel.NineGridBrush

local NINE_GRID_OPTION_TYPES = {
   brush = 'string',
   nine_grid_gradiant = 'table',
   nine_grid_slope = 'number',
   nine_grid_max_height = 'number',
}

local NG_STRING_TO_DEGREES = {
   front  = 1,
   left   = 2,
   back   = 3,
   right  = 4,
}

local NG_ROTATION_TABLE = { 
   "front",    -- 0 degrees.
   "left",     -- 90 degress.
   "back",     -- 180 degress.
   "right",    -- 270 degress.
   "front",    -- 360 degress.
   "left",     -- 450 degress.
   "back",     -- 540 degress.
   "right",    -- 630 degress.
   "front",    -- 720 degrees (longer than it needs to be, frankly).
}


-- called to initialize the component on creation and loading.
--
function Roof:initialize(entity, json)
   self._entity = entity
end

-- make the destination region match the shape of the column.  call this
-- manually after calling a method which could change the shape of the
-- roof.
--
function Roof:layout()
   local collsion_shape = self:_compute_collision_shape()                              

   self._entity:get_component('destination')
                  :get_region()
                     :modify(function(cursor)
                           cursor:copy_region(collsion_shape)
                        end)
   return self
end

-- set the xz cross section of the roof.  a roof gets its height based
-- on the voxel brush and the nine-grid pain mode of the stonehearth:
-- construction_data component.  make sure to call :layout() to actually
-- change the region.
--
function Roof:cover_region2(brush, region2)
   checks('self', 'string', 'Region2')

   self._sv.brush = brush
   self._sv._nine_grid_region = region2
   self.__saved_variables:mark_changed()
   return self
end

function Roof:_compute_collision_shape()
   local brush = self._sv.brush
   local region = self._sv._nine_grid_region
   local slope = self._sv.nine_grid_slope
   local max_height = self._sv.nine_grid_max_height
   local y_offset = self._sv.nine_grid_y_offset
   local gradiant = self._sv.nine_grid_gradiant

   -- xxx: i wish we didnt have to do all this fixup nonsense.  fixing
   -- it requires being able to convert json back to value types, which
   -- is somewhat tricky...
   --[[
   if type(nine_grid_region) == 'table' then
      local t = nine_grid_region
      nine_grid_region = Region2()
      nine_grid_region:load(t)
   end
   ]]

   local brush = _radiant.voxel.create_nine_grid_brush(brush)
                                 :set_slope(slope or 1)

   if max_height then
      brush:set_max_height(max_height)
   end
   if y_offset then
      brush:set_y_offset(y_offset)
   end
   if gradiant then
      local flags = 0
      for _, f in ipairs(gradiant) do
         if f == "front" then
            flags = flags + NineGridBrush.Front
         elseif f == "back" then
            flags = flags + NineGridBrush.Back
         elseif f == "left" then
            flags = flags + NineGridBrush.Left
         elseif f == "right" then
            flags = flags + NineGridBrush.Right
         end
      end
      brush:set_gradiant_flags(flags)
            :set_clip_whitespace(true)
   end

   --[[
   if construction_data.grid9_tile_mode then
      brush:set_grid9_tile_mode(construction_data.grid9_tile_mode)
   end
   if construction_data.grid9_slope_mode then
      brush:set_grid9_tile_mode(construction_data.grid9_slope_mode)
   end
   ]]

   -- break the roof up into multiple, non-overlapping regions
   -- before computing the shape.  this ensures that things that don't
   -- look connected aren't.  you could argue we should divide this into
   -- multiple roof entities, but that's harder than doing this (and this
   -- is a strict improvement, so let's try it) -- tony
   local shape = Region3()
   local regions = csg_lib.get_contiguous_regions(region)
   for _, region in pairs(regions) do
      region = region:inflated(Point2(1, 1))
      local s = brush:set_grid_shape(region)
                     :paint_once()
      shape:add_region(s)
   end

   return shape
end

-- changes properties in the construction data component.
-- 
--    @param options - the options to change.  
--
function Roof:apply_nine_grid_options(options)
   if options then
      for name, val in pairs(options) do
         if NINE_GRID_OPTION_TYPES[name] == 'number' then
            self._sv[name] = tonumber(val)
         elseif NINE_GRID_OPTION_TYPES[name] ~= nil then
            self._sv[name] = val
         end
      end
      self.__saved_variables:mark_changed()
   end
   return self
end

function Roof:save_to_template()
   local result = {
      brush = self._sv.brush,
      nine_grid_region = self._sv._nine_grid_region,
      nine_grid_slope = self._sv.nine_grid_slope,
      nine_grid_gradiant = self._sv.nine_grid_gradiant,
      nine_grid_max_height = self._sv.nine_grid_max_height,      
   }
   return result
end

function Roof:load_from_template(data, options, entity_map)
   self._sv.brush = data.brush
   self._sv.nine_grid_slope = data.nine_grid_slope
   self._sv.nine_grid_gradiant = data.nine_grid_gradiant
   self._sv.nine_grid_max_height = data.nine_grid_max_height
   self._sv._nine_grid_region = Region2()
   self._sv._nine_grid_region:load(data.nine_grid_region)
   self:apply_nine_grid_options(data)
end

function Roof:rotate_structure(degrees)
   build_util.rotate_structure(self._entity, degrees)

   if self._sv._nine_grid_region then
      local cursor = self._sv._nine_grid_region
      local origin = Point2(0.5, 0.5)
      cursor:translate(-origin)
      cursor:rotate(degrees)
      cursor:translate(origin)      
   end
   if self._sv.nine_grid_gradiant then
      local new_gradiant = {}
      local rotation_offset = degrees / 90
      for _, value in pairs(self._sv.nine_grid_gradiant) do
         local rotation = NG_STRING_TO_DEGREES[value]
         local next_value = NG_ROTATION_TABLE[rotation + rotation_offset]
         table.insert(new_gradiant, next_value)
      end
      self._sv.nine_grid_gradiant = new_gradiant
   end  
end

function Roof:clone_from(entity)
   if entity then
      local into = self._sv
      local from = entity:get_component('stonehearth:construction_data')._sv

      into._nine_grid_region = from._nine_grid_region and Region2(from._nine_grid_region) or nil
      into.nine_grid_slope = from.nine_grid_slope
      into.nine_grid_gradiant = from.nine_grid_gradiant
      into.nine_grid_max_height = from.nine_grid_max_height
      self.__saved_variables:mark_changed()
   end

   return self
end

-- begin editing the column pointed to by `other_column`.  basically just
-- copy the shape and important variables in the save state
function Roof:begin_editing(entity)
   self._editing_region = _radiant.client.alloc_region3()
   return self
end

-- return the 'editing region' for the column.  the editing region is a
-- region which fully covers the shape of the structure, minus all the
-- fixture and portals
--
function Roof:get_editing_reserved_region()
   return self._editing_region
end

return Roof
