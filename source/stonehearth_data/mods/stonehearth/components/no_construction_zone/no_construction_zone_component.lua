local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Cube3 = _radiant.csg.Cube3
local TraceCategories = _radiant.dm.TraceCategories

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
   self._traces = {}

   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.structures = {}
      self._sv.overlapping = {}
      self.__saved_variables:mark_changed()
      self._region = _radiant.sim.alloc_region3()
      entity:add_component('region_collision_shape')
                  :set_region(self._region)
                  :set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)
   else
      self._region = entity:get_component('region_collision_shape'):get_region()
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
            for id, entry in pairs(self._sv.structures) do
               self:_trace_structure(id, entry.structure)
            end
         end)
   end
end

function NoConstructionZoneComponent:destroy()
   self._dirty = false
   self:clear_traces()
end

function NoConstructionZoneComponent:set_building_entity(entity)
   self._sv.building_entity = entity
   self.__saved_variables:mark_changed()
end

function NoConstructionZoneComponent:get_building_entity()
   return self._sv.building_entity
end

function NoConstructionZoneComponent:add_structure(structure)
   assert(self._sv.building_entity)
   local id = structure:get_id()

   if not self._sv.structures[id] then
      local destination = structure:get_component('destination')
      if destination then
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

         self._sv.structures[id] = {
            region = region,
            structure = structure,
            structure_type = structure_type
         }
         self.__saved_variables:mark_changed()

         self:_trace_structure(id, structure)
         self:_mark_dirty()
      end
   end
end

function NoConstructionZoneComponent:remove_structure(id)
   if self._sv.structures[id] then
      self._sv.structures[id] = nil
      self.__saved_variables:mark_changed()
      self:_mark_dirty()
   end
end

-- Called when the user starts construction; we no longer need to care about update the
-- zone, because it will never be updated!
function NoConstructionZoneComponent:clear_traces()
   for _, traces in pairs(self._traces) do
      for _, trace in pairs(traces) do
         trace:destroy()
      end
   end
end

function NoConstructionZoneComponent:_trace_structure(id, structure)
   if not self._traces[id] then
      local trace
      local traces = {}
      self._traces[id] = traces

      trace = structure:add_component('stonehearth:construction_data'):trace_data('create no-construction zone', TraceCategories.SYNC_TRACE)
                                          :on_changed(function()
                                                self:_mark_dirty()
                                             end)
      table.insert(traces, trace)

      trace = structure:add_component('mob'):trace('create no-construction zone', TraceCategories.SYNC_TRACE)
                                          :on_changed(function()
                                                self:_mark_dirty()
                                             end)
      table.insert(traces, trace)

      local destination = structure:get_component('destination')
      trace = destination:trace_region('create no-construction zone', TraceCategories.SYNC_TRACE)
                                          :on_changed(function()
                                                self:_mark_dirty()
                                             end)
                                          
      table.insert(traces, trace)
   end
end

function NoConstructionZoneComponent:_mark_dirty()
   if stonehearth.build:in_transaction() then
      if not self._dirty then
         self._dirty = true      
         stonehearth.build:at_end_of_transaction(function()
               self:_update_no_construction_shape()
               self._dirty = false
            end)
      end
   else
      self:_update_no_construction_shape()
      self._dirty = false      
   end
end

function NoConstructionZoneComponent:_update_no_construction_shape()
   local origin = radiant.entities.get_world_grid_location(self._sv.building_entity)
   
   -- set our region_collision_shape to the envelope encompassing all the space
   -- which must remain "empty" to build the building
   self._region:modify(function(cursor)
         cursor:clear()
         if not origin then
            -- if get_world_grid_location returns nil, we must not be a descendant of the root.
            -- this routinely happens when the building is unlinked due to a merge or undo
            -- step.  no biggie, just bail.
            return
         end
         for id, entry in pairs(self._sv.structures) do
            local structure, structure_type = entry.structure, entry.structure_type
            local position = radiant.entities.get_world_grid_location(entry.structure)
            local offset = position - origin
            
            for cube in entry.region:get():each_cube() do
               cube:translate(offset)
               self:_add_slice_for_cube(structure, structure_type, cube, cursor)
            end
         end
      end)

   self:_update_overlapping_structures()
end

function NoConstructionZoneComponent:_update_overlapping_structures()
   local current_overlap = {}

   if not self._region:get():empty() then
      local world_region = radiant.entities.local_to_world(self._region:get(), self._entity)

      local potenial_overlaps = radiant.terrain.get_entities_in_region(world_region)
      for id, candidate in pairs(potenial_overlaps) do
         -- buildings whose ncz may overlap ours
         local candidate_ncz = candidate:get_component('stonehearth:no_construction_zone')
         if candidate_ncz then
            local building = candidate_ncz:get_building_entity()
            if building ~= self._sv.building_entity then
               current_overlap[id] = candidate
            end
         end

         -- ignore structures.  we've already accounted from them with the no-construction-zone
         -- test, above.
         if not candidate:get_component('stonehearth:construction_data') then
            -- big solid blocks of something (trees, cliffs, etc.)
            local rcs = candidate:get_component('region_collision_shape')
            if rcs and rcs:get_region_collision_type() ~= _radiant.om.RegionCollisionShape.NONE then
               current_overlap[id] = candidate
            end
         end
      end
   end
   
   -- add the overlap flag to things which we are newly overlapping
   for id, overlapping in pairs(current_overlap) do
      if not self._sv.overlapping[id] then
         self:_twiddle_overlap_bit(id, overlapping)
         local ncz = overlapping:get_component('stonehearth:no_construction_zone')
         if ncz then
            ncz:_twiddle_overlap_bit(self._entity:get_id(), self._entity)
         end
      end
   end
   -- remove the overlap flag from things which we no longer overlap
   for id, overlapping in pairs(self._sv.overlapping) do
      if not current_overlap[id] then
         self:_twiddle_overlap_bit(id, nil)
         local ncz = overlapping:get_component('stonehearth:no_construction_zone')
         if ncz then
            ncz:_twiddle_overlap_bit(self._entity:get_id(), nil)
         end
      end
   end
end

function NoConstructionZoneComponent:_twiddle_overlap_bit(key, value)
   if self._sv.overlapping[key] ~= value then
      self._sv.overlapping[key] = value
      self.__saved_variables:mark_changed()
   end
end

function NoConstructionZoneComponent:_add_slice_for_cube(structure, structure_type, cube, region)
   if structure_type == FLOOR then
      region:add_cube(cube)
   elseif structure_type == COLUMN then
      -- ug.  need to know the walls that the column is connected to and inflate in the
      -- direction of their normals, too.
      region:add_cube(cube)
   elseif structure_type == WALL then
      local normal = structure:get_component('stonehearth:construction_data')
                                 :get_normal()
                                 :scaled(2)
      if normal.x < 0 or normal.z < 0 then
         region:add_cube(Cube3(cube.min + normal, cube.max))
      else
         region:add_cube(Cube3(cube.min, cube.max + normal))
      end
   end
end

return NoConstructionZoneComponent

