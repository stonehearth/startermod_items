local stonehearth_building = require 'stonehearth_building'

local BlueprintComponent = class()

function BlueprintComponent:__init(entity)
   self._entity = entity
   self._blueprints = {}
end

function BlueprintComponent:extend(json)
   self._structure_path = json.structure_path
   self._component_name = json.component_name
end

function BlueprintComponent:add_blueprint(blueprint)
   table.insert(self._blueprints, blueprint)
end

function BlueprintComponent:create_structure(parent_entity)
   -- clone...
   local structure = radiant.entities.create_entity(self._structure_path)
   local component = structure:get_component(self._component_name)
   local bc = structure:get_component(self._component_name .. '_blueprint')
   
   component:initialize_from_blueprint(bc)

   for _, blueprint in ipairs(self._blueprints) do
      blueprint:create_structure(structure)
   end
   return structure
end

return BlueprintComponent
