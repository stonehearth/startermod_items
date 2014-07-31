--[[
   Use this component to denote an entity that the player has
   placed in the world, but isn't actully yet constructed or placed
   by the people in the world.
   TODO: Only show these items in a special mode (build mode)
]]

local GhostFormComponent = class()

function GhostFormComponent:initialize(entity, json)
   self._entity = entity

   assert(not next(json), 'iconic_form components should not be specified in json files')
   if not self._sv.root_entity then
      self._entity:set_debug_text(self._entity:get_debug_text() .. ' (ghost)')
   end
end

function GhostFormComponent:get_root_entity()
   return self._sv.root_entity
end

function GhostFormComponent:set_root_entity(root_entity)
   assert(not self._sv.root_entity, 'root entity should be initialized exactly once')
   self._sv.root_entity = root_entity
   self.__saved_variables:mark_changed()

   -- some trivial error checking
   local uri = self._entity:get_uri()
   local function verify_no_component(name)
      assert(not self._entity:get_component(name), string.format('ghost entity %s should not have a %s component.', uri, name))
   end
   verify_no_component('sensor')
   verify_no_component('destination')
   verify_no_component('region_collision_shape')

   return self
end

function GhostFormComponent:get_iconic_entity()
   return self._sv.iconic_entity
end

function GhostFormComponent:set_iconic_entity(iconic_entity)
   assert(not self._sv.iconic_entity, 'iconic entity should be initialized exactly once')
   self._sv.iconic_entity = iconic_entity
   self.__saved_variables:mark_changed()
   return self
end

function GhostFormComponent:set_placeable(placeable)
   if placeable then
      -- add the command to place the item from the proxy
      self._entity:add_component('stonehearth:commands')
                     :add_command('/stonehearth/data/commands/move_item')
   end
   return self
end

return GhostFormComponent
