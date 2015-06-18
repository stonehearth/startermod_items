stonehearth_common = {}

local function destroy_entity_raw(entity)
   if not entity or not entity:is_valid() then
      return
   end

   if radiant.is_server then
      _radiant.sim.destroy_entity(entity)
   else
      _radiant.client.destroy_authoring_entity(entity:get_id())
   end
end

function stonehearth_common.destroy_child_entities(entity)
   -- destroy all the children when destorying the parent.  should we do
   -- this from c++?  if not, entities which get destroyed from the cpp
   -- layer won't get this behavior.  maybe that's just impossible (i.e. forbid
   -- it from happening, since the cpp layer knowns nothing about game logic?)
   local ec = entity:get_component('entity_container')
   if ec then
      -- Copy the blueprint's (container's) children into a local var first, because
      -- _set_teardown_recursive could cause the entity container to be invalidated.
      local ec_children = {}
      for id, child in ec:each_child() do
         ec_children[id] = child
      end         
      for id, child in pairs(ec_children) do
         radiant.entities.destroy_entity(child)
      end
   end   
end

function stonehearth_common.destroy_entity_forms(entity)
   --If we're the big one, destroy the little and ghost one
   local entity_forms = entity:get_component('stonehearth:entity_forms')
   if entity_forms then
      local iconic_entity = entity_forms:get_iconic_entity()
      if iconic_entity then
         destroy_entity_raw(iconic_entity)
      end
      local ghost_entity = entity_forms:get_ghost_entity()
      if ghost_entity then
         destroy_entity_raw(ghost_entity)
      end
   end

   --If we're the little one, call destroy on the big one and exit
   local iconic_component = entity:get_component('stonehearth:iconic_form')
   if iconic_component then
      local full_sized = iconic_component:get_root_entity()
      radiant.entities.destroy_entity(full_sized)
      return 
   end
end

return stonehearth_common
