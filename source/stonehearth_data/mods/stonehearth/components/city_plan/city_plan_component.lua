local Color4 = _radiant.csg.Color4
local CityPlanComponent = class()

function CityPlanComponent:__init(entity, data_binding)
   self._entity = entity
   self._data_binding = data_binding
end

function CityPlanComponent:__tojson()
   return {}
end

function CityPlanComponent:extend(json)
end

function CityPlanComponent:add_blueprint(blueprint, location)
   -- create a new fabricator...   to... you know... fabricate
   local project = self:_create_fabricator(blueprint)
   radiant.terrain.place_entity(project, location)
end

function CityPlanComponent:_create_fabricator(blueprint)
   -- either you're a fabricator or you contain things which may be fabricators.  not
   -- both!
   local fabricator = radiant.entities.create_entity()
   self:_init_fabricator(fabricator, blueprint)
   self:_init_fabricator_children(fabricator, blueprint)
   return fabricator
end

function CityPlanComponent:_init_fabricator(fabricator, blueprint)
   local blueprint_mob = blueprint:add_component('mob')
   local parent = blueprint_mob:get_parent()
   if parent then
      parent:add_component('entity_container'):add_child(fabricator)
   end
   local transform = blueprint_mob:get_transform()
   fabricator:add_component('mob'):set_transform(transform)
   
   fabricator:set_debug_text('fabricator for ' .. blueprint:get_debug_text())
   
   if blueprint:get_component('stonehearth:construction_data') then
      local name = tostring(blueprint)
      fabricator:add_component('stonehearth:fabricator')
                     :start_project(name, blueprint)
      fabricator:add_component('render_info')
                     :set_material('materials/blueprint_gridlines.xml')
                     --:set_model_mode('opaque')
   end
end

function CityPlanComponent:_init_fabricator_children(fabricator, blueprint)
   local ec = blueprint:get_component('entity_container')  
   if ec then
      for id, child in ec:each_child() do
         local fc = self:_create_fabricator(child)
         fabricator:add_component('entity_container'):add_child(fc)
      end
   end
end

return CityPlanComponent
