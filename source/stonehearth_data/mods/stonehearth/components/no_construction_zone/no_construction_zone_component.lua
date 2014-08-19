local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2

local NoConstructionZoneComponent = class()

local WALL = 'stonehearth:wall'
local COLUMN = 'stonehearth:column'
local ROOF = 'stonehearth:roof'
local FLOOR = 'stonehearth:floor'
local STRUCTURE_TYPES = {
   WALL,
   COLUMN,
   FLOOR,
}

function NoConstructionZoneComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   self._sv.region = _radiant.sim.alloc_region2()
   self._region_traces = {}
   self._mob_traces = {}
   self._structures = {}
end

function NoConstructionZoneComponent:add_structure(structure)
   local destination = structure:get_component('destination')
   if destination then
      local id = structure:get_id()
      local region = destination:get_region()
      local structure_type
      for _, t in ipairs(STRUCTURE_TYPES) do
         if structure:get_component(t) then
            structure_type = t
            break
         end
      end
      if not structure_type then
         return
      end

      local traces = {}
      self._structures[id] = {
         traces = traces,
         region = region,
         structure = structure,
         structure_type = structure_type
      }

      local function mark_dirty()
         self:_mark_dirty()
      end

      local trace
      trace = structure:add_component('stonehearth:construction_data'):trace_data('create no-construction zone')
                                          :on_changed(mark_dirty)
      table.insert(traces, trace)

      trace = structure:add_component('mob'):trace('create no-construction zone')
                                          :on_changed(mark_dirty)
      table.insert(traces, trace)

      trace = destination:trace_region('create no-construction zone')
                                          :on_changed(mark_dirty)
                                          :push_object_state()
      table.insert(traces, trace)
   end
end

function NoConstructionZoneComponent:_mark_dirty()
   if not self._dirty then
      self._dirty = true
      radiant.events.listen_once(radiant, 'stonehearth:gameloop', function()
            self:_update_shape()
            self._dirty = false
         end)
   end
end

function NoConstructionZoneComponent:_update_shape()
   local origin = radiant.entities.get_world_grid_location(self._entity)
   self._sv.region:modify(function(cursor)
         cursor:clear()
         for id, entry in pairs(self._structures) do
            local structure, structure_type = entry.structure, entry.structure_type
            local position = radiant.entities.get_world_grid_location(entry.structure)
            local offset = position - origin
            
            for cube in entry.region:get():each_cube() do
               self:_add_slice_for_cube(structure, structure_type, cube:translated(offset), cursor)
            end
         end
      end)
end

function NoConstructionZoneComponent:_add_slice_for_cube(structure, structure_type, cube, region)
   if structure_type == FLOOR then
      region:add_cube(Rect2(Point2(cube.min.x, cube.min.z),
                            Point2(cube.max.x, cube.max.z)))
   elseif structure_type == COLUMN then
      -- ug.  need to know the walls that the column is connected to and inflate in the
      -- direction of their normals, too.
      region:add_cube(Rect2(Point2(cube.min.x, cube.min.z),
                            Point2(cube.max.x, cube.max.z)))
   elseif structure_type == WALL then
      local normal = structure:get_component('stonehearth:construction_data')
                                 :get_normal()
                                 :scaled(2)
      if normal.x < 0 or normal.z < 0 then
         region:add_cube(Rect2(Point2(cube.min.x + normal.x, cube.min.z + normal.z),
                               Point2(cube.max.x, cube.max.z)))
      else
         region:add_cube(Rect2(Point2(cube.min.x, cube.min.z),
                               Point2(cube.max.x + normal.x, cube.max.z + normal.z)))
      end
   end
end

return NoConstructionZoneComponent

