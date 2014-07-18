local constants = require('constants').construction

local Roof = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3


-- called to initialize the component on creation and loading.
--
function Roof:initialize(entity, json)
   self._entity = entity
   self._trace = self._entity:get_component('stonehearth:construction_data'):trace_data('layout roof')
                     :on_changed(function()
                           self:layout()
                        end)

   if not self._sv.connected_to then
      self._sv.connected_to = {}
   end
end

function Roof:destroy()
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

-- make the destination region match the shape of the column.  call this
-- manually after calling a method which could change the shape of the
-- roof.
--
function Roof:layout()
   local collsion_shape = self._entity:get_component('stonehearth:construction_data')
                                          :create_voxel_brush()
                                          :paint_once()

   self._entity:get_component('destination')
                  :get_region()
                     :modify(function(cursor)
                           cursor:copy_region(collsion_shape)
                        end)

   -- since our shape has likely changed, ask all the structures connected
   -- to us to layout themselves, too.
   for id, entry in pairs(self._sv.connected_to) do
      if entry.entity then
         entry.entity:get_component(entry.component_name)
                        :layout()
      end
   end

   return self
end

-- set the xz cross section of the roof.  a roof gets its height based
-- on the voxel brush and the nine-grid pain mode of the stonehearth:
-- construction_data component.  make sure to call :layout() to actually
-- change the region.
--
function Roof:cover_region2(region2)
   self._entity:get_component('stonehearth:construction_data')
                  :set_nine_grid_region2(region2)
   return self
end

-- connect the roof to the specified structure.  whenever the shape of
-- the roof changes, we'll change the shape of this structure, too.
--
--    @param entity - the entity connected to
--    @param component_name - the name of the component which contains the
--                            entity's structure (e.g. stonehearth:wall)
--
function Roof:connect_to_structure(entity, component_name)
   self._sv.connected_to[entity:get_id()] = {
      entity = entity,
      component_name = component_name,
   }
end

return Roof
