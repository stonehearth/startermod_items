local Proxy = require 'services.build.proxy'
local ProxyContainer = class(Proxy)
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

function ProxyContainer:__init(parent_proxy)
   self[Proxy]:__init(self, parent_proxy, nil, nil)
   self._rgn2 = _radiant.client.alloc_region2()

   self._all_children = {}
   self:_trace_all_children(self:get_entity())
   self:get_entity():add_component('stonehearth:no_construction_zone')

   self:_rebuild_zone()
end

function ProxyContainer:_trace_all_children(entity)
   self._all_children[entity:get_id()] = self:_trace_entity(entity)

   local ec = entity:get_component('entity_container')
   if ec then
      for id, child in ec:each_child() do
         self:_trace_all_children(child)
      end
   end   
end

function ProxyContainer:_trace_entity(entity)
   local id = entity:get_id()
   local result = self._all_children[id]
   if not result then
      result = {}      
      local mob = entity:add_component('mob')
      result.mob_promise = mob:trace_object_changes('no construnction zone')
                                 :on_modified(function ( ... )
                                    self:_move_child(entity)                              
                                 end)

      result.entity = entity
      local ec = entity:add_component('entity_container')
      if ec then
         result.ec_promise = ec:trace_children('no construction zone')
                                                :on_added(function(...)
                                                   self:_add_child(...)
                                                end)
                                                :on_removed(function(...)
                                                   self:_remove_child(...)
                                                end)
      end
      self._all_children[id] = result
   end
   return result
end

function ProxyContainer:_remove_all_children(id)
   local info = self._all_children[id]
   if info then
      info.mob_promise:destroy()
      info.ec_promise:destroy()
      self._all_children[id] = nil
      if info.entity then
         local ec = entity:get_component('entity_container')
         if ec then
            for id, child in ec:each_child() do
               self:_remove_all_children(id)
            end
         end
      end
   end

end

function ProxyContainer:_move_child(entity)
   self:_rebuild_zone()
end

function ProxyContainer:_add_child(id, entity)
   self:_trace_all_children(entity)
   self:_rebuild_zone()
end

function ProxyContainer:_remove_child(id)
   self:_remove_all_children(id)
   self:_rebuild_zone()
end

function ProxyContainer:_add_nobuild_zone(entity, origin, cursor)
   local fabinfo = entity:get_component_data('stonehearth:construction_data')
   if fabinfo then
      local offset = entity:get_component('mob'):get_world_grid_location() - origin
      local region = entity:get_component('destination'):get_region():get()

      offset = Point2(offset.x, offset.z)
      region = region:project_onto_xz_plane()
      
      cursor:add_region(region:translated(offset))
      if fabinfo.normal then
         local normal = Point2(fabinfo.normal.x, fabinfo.normal.z)         
         cursor:add_region(region:translated(offset + normal))
         if fabinfo.needs_scaffolding then
            cursor:add_region(region:translated(offset + normal:scaled(2)))
         end
      end
      if fabinfo.connected_to then
         local grow_size = Point2(1, 1)
         local zone_offset = Point2(0, 0)
         for _, obj in ipairs(fabinfo.connected_to) do
            local oci = obj:get_component_data('stonehearth:construction_data')
            if oci and oci.normal then
               if zone_offset.x == 0 then zone_offset.x = oci.normal.x end
               if zone_offset.y == 0 then zone_offset.y = oci.normal.z end
            end
         end
         cursor:add_region(region:translated(offset + zone_offset):inflated(grow_size))
      end
   end
end

function ProxyContainer:_rebuild_zone()
   local origin = self:get_entity():get_component('mob'):get_world_grid_location()
   self._rgn2:modify(function(cursor)
      cursor:clear()   
      for id, info in pairs(self._all_children) do
         if info.entity then
            self:_add_nobuild_zone(info.entity, origin, cursor)
         end
      end
   end)
   self:get_entity():set_component_data('stonehearth:no_construction_zone', { region2 = self._rgn2 })
end

return ProxyContainer
