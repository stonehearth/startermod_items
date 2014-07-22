 constants = require('constants').construction

local Building = class()
local Rect2 = _radiant.csg.Rect2
local Cube3 = _radiant.csg.Cube3
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3

local INFINITE = 10000000
local WALL = 'stonehearth:wall'
local COLUMN = 'stonehearth:columns'
local ROOF = 'stonehearth:roof'
local FLOOR = 'stonehearth:floor'

local STRUCTURE_TYPES = {
   WALL,
   COLUMN,
   ROOF,
   FLOOR,
}

function Building:initialize(entity, json)
   self._entity = entity
   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.structures = {}
      for _, structure_type in pairs(STRUCTURE_TYPES) do
         self._sv.structures[structure_type] = {}
      end
   else
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
            for _, entry in pairs(self._roofs) do
               self:_trace_roof(entry.entity)
            end
         end)      
   end
   self._traces = {}
   self._structures = self._sv.structures
   self._walls = self._structures[WALL]
   self._roofs = self._structures[ROOF]
   self._floors = self._structures[FLOOR]
   self._columns = self._structures[COLUMN]
end

function Building:calculate_floor_region()
   local floor_region = Region2()
   for _, entry in pairs(self._floors) do
      local floor_entity = entry.entity
      local rgn = floor_entity:get_component('destination'):get_region():get()
      for cube in rgn:each_cube() do
         local rect = Rect2(Point2(cube.min.x, cube.min.z),
                            Point2(cube.max.x, cube.max.z))
         floor_region:add_unique_cube(rect)
      end
   end

   -- we need to construct the most optimal region possible.  since floors are colored,
   -- the floor_region may be extremely complex (at worst, 1 cube per point!).  to get
   -- the best result, iterate through all the points in the region in an order in which
   -- the merge-on-add code in the region backend will build an extremly optimal region.
   local optimized_region = Region2()
   local bounds = floor_region:get_bounds()
   local pt = Point2()
   local try_merge = false
   for x=bounds.min.x,bounds.max.x do
      pt.x = x
      for y=bounds.min.y,bounds.max.y do
         pt.y = y
         if floor_region:contains(pt) then
            optimized_region:add_point(pt)
            if try_merge then
               optimized_region:optimize_by_merge()
            end
         else
            try_merge = true
         end
      end
   end

   return optimized_region
end

function Building:add_structure(entity)
   local id = entity:get_id()

   for _, structure_type in pairs(STRUCTURE_TYPES) do
      local trace
      local structure = entity:get_component(structure_type)

      if structure then
         self._sv.structures[structure_type][id] = {
            entity = entity,
            structure = structure
         }
         if structure_type == ROOF then
            self:_add_roof(entity)
            trace = self:_trace_roof(entity)
         elseif structure_type == FLOOR then
            self:_add_floor(entity)
         elseif structure_type == WALL then
            self:_add_wall(entity)
         end

         if trace then
            trace:push_object_state()
         end
         structure:layout()
         break
      end
   end
   self.__saved_variables:mark_changed()
end

function Building:_add_wall(wall)
   for _, entry in pairs(self._floors) do
      local floor = entry.entity
      wall:get_component('stonehearth:construction_progress')
               :add_dependency(floor)
   end
end

function Building:_add_floor(floor)
   for _, entry in pairs(self._walls) do
      local wall = entry.entity
      wall:get_component('stonehearth:construction_progress')
               :add_dependency(floor)
   end
end

function Building:_add_roof(roof)
   for _, entry in pairs(self._walls) do
      local structure = entry.entity

      -- don't build the roof until we've built all the supporting structures
      roof:add_component('stonehearth:construction_progress')
                  :add_dependency(structure)

      -- loan out some scaffolding
      structure:get_component('stonehearth:construction_data')
                  :loan_scaffolding_to(roof)
   end
end

function Building:_trace_roof(roof)
   -- when the roof construction data changes, layout the roof!
   local trace = roof:get_component('stonehearth:construction_data'):trace_data('layout roof')
                        :on_changed(function()
                              self:layout_roof(roof)
                           end)
                           
   local id = roof:get_id()
   if not self._traces[id] then
      self._traces[id] = {}
   end
   table.insert(self._traces[id], trace)
   return trace
end

function Building:grow_local_box_to_roof(entity, local_box)
   local p0, p1 = local_box.min, local_box.max

   local shape = Region3(local_box)
   for _, entry in pairs(self._roofs) do
      local roof = entry.entity
      local roof_region = roof:get_component('destination'):get_region():get()

      local stencil = Cube3(Point3(p0.x, p1.y, p0.z),
                            Point3(p1.x, INFINITE, p1.z))
      
      -- translate the stencil into the roof's coordinate system, clip it,
      -- then transform the result back to our coordinate system
      local offset = radiant.entities.get_location_aligned(roof) -
                     radiant.entities.get_location_aligned(entity)

      local roof_overhang = roof_region:clipped(stencil:translated(-offset))                                       
                                       :translated(offset)
      
      -- iterate through each "shingle" in the overhang, growing the shape
      -- upwards toward the base of the shingle.
      for shingle in roof_overhang:each_cube() do
         local col = Cube3(Point3(shingle.min.x, p1.y, shingle.min.z),
                           Point3(shingle.max.x, shingle.min.y, shingle.max.z))
         shape:add_unique_cube(col)
      end
   end
   return shape
end

-- links will be put back by the BuildingUndoManager
function Building:unlink_entity(entity)
   for _, structure_type in pairs(STRUCTURE_TYPES) do
      local structure = entity:get_component(structure_type)
      if structure then
         self._sv.structures[structure_type][id] = nil
         if structure_type == ROOF then            
            self:layout_roof()
         end
         break
      end
   end
   self.__saved_variables:mark_changed()
end

function Building:layout_roof(entity)
   -- first, layout the roof
   if entity then
      entity:get_component(ROOF)
                  :layout()
   end

   -- now layout all the walls and columns
   for _, entry in pairs(self._columns) do
      entry.structure:layout()
   end
   for _, entry in pairs(self._walls) do
      entry.structure:layout()
   end
end

return Building
