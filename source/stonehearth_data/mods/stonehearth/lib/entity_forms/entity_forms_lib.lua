local Entity = _radiant.om.Entity

local entity_forms_lib = {}

function entity_forms_lib.get_uris(uri_or_entity)
   local root, iconic, ghost
   
   if type(uri_or_entity) == 'string' then
      local data = radiant.entities.get_component_data(uri_or_entity, 'stonehearth:entity_forms')
      return uri_or_entity,
             data and data.iconic_form,
             data and data.ghost_form
   end

   if radiant.util.is_a(uri_or_entity, Entity) then
      local entity = uri_or_entity
      local ef = entity:get_component('stonehearth:entity_forms')
      if not ef then
         local iconic = entity:get_component('stonehearth:iconic_form')
         if iconic then
            entity = iconic:get_root_entity()
            ef = entity:get_component('stonehearth:entity_forms')
         else
            local ghost = entity:get_component('stonehearth:ghost_form')
            if ghost then
               entity = iconic:get_root_entity()
               ef = entity:get_component('stonehearth:entity_forms')
            else
               return entity:get_uri(), nil, nil  -- just a plain old entity with no forms
            end
         end
      end
   
      local iconic_entity = ef:get_iconic_entity()
      local ghost_entity = ef:get_ghost_entity()
      return entity:get_uri(),
             iconic_entity and iconic_entity:get_uri(),
             ghost_entity and ghost_entity:get_uri()
   end
end

function entity_forms_lib.get_forms(entity)
   local root, iconic, ghost
   
   local entity_forms = entity:get_component('stonehearth:entity_forms')
   if entity_forms then
      return entity, entity_forms:get_iconic_entity(), entity_forms:get_ghost_entity()
   end

   local iconic_form = entity:get_component('stonehearth:iconic_form')
   if iconic_form then
      return entity_forms_lib.get_forms(iconic_form:get_root_entity())
   end

   local ghost_form = entity:get_component('stonehearth:ghost_form')
   if ghost_form then
      return entity_forms_lib.get_forms(ghost_form:get_root_entity())
   end
end

function entity_forms_lib.get_root_entity(entity)
   local entity_forms = entity:get_component('stonehearth:entity_forms')
   if entity_forms then
      return entity
   end

   local iconic_form = entity:get_component('stonehearth:iconic_form')
   if iconic_form then
      return iconic_form:get_root_entity()
   end

   local ghost_form = entity:get_component('stonehearth:ghost_form')
   if ghost_form then
      return ghost_form:get_root_entity()
   end
end

return entity_forms_lib
