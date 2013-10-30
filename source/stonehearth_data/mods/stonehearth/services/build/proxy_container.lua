local Proxy = require 'services.build.proxy'
local ProxyContainer = class(Proxy)
local Point3 = _radiant.csg.Point3

function ProxyContainer:__init(parent_proxy)
   self[Proxy]:__init(self, parent_proxy, nil, nil)
   self._rgn = _radiant.client.alloc_region()

   self._all_children = {}
   self:_trace_all_children(self:get_entity())
   self:get_entity():add_component('stonehearth:no_construction_zone')

   self:_rebuild_zone()
end

function ProxyContainer:_trace_all_children(entity)
   self._all_children[entity:get_id()] = self:_trace_entity(entity)

   local ec = entity:get_component('entity_container')
   if ec then
      for id, child in ec:get_children():items() do
         self:_trace_all_children(entity)
      end
   end   
end

function ProxyContainer:_trace_entity(entity)
   local id = entity:get_id()
   local result = self._all_children[id]
   if not result then
      result = {}      
      local mob = entity:add_component('mob')
      result.mob_promise = mob:trace('no construnction zone')
                                 :on_changed(function ( ... )
                                    self:_move_child(entity)                              
                                 end)

      result.entity = entity
      local ec = entity:add_component('entity_container')
      if ec then
         result.ec_promise = ec:get_children():trace('no construction zone')
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
            for id, child in ec:get_children():items() do
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
   self:_remove_all_children(entity)
   self:_rebuild_zone()
end

function ProxyContainer:_rebuild_zone()
   local grow_size = Point3(2, 0, 2)
   local origin = self:get_entity():get_component('mob'):get_world_grid_location()

   local cursor = self._rgn:modify()
   cursor:clear()
   
   for id, info in pairs(self._all_children) do
      if info.entity then
         local fi = radiant.entities.get_entity_data(info.entity, 'stonehearth:fabricator_info')
         if fi then
            local offset = info.entity:get_component('mob'):get_world_grid_location()
            local br = info.entity:get_component('destination'):get_region():get()
            cursor:add_region(br:translated(offset - origin):inflated(grow_size));
         end
      end
   end
   self:get_entity():set_component_data('stonehearth:no_construction_zone', { region = self._rgn })
end

return ProxyContainer
