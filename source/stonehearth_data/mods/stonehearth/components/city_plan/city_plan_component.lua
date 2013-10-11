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

function CityPlanComponent:add_blueprint(location, blueprint)
   -- create a new fabricator...   to... you know... fabricate
   local project = self:_create_project_recursive(blueprint)
   radiant.terrain.place_entity(project, location)
end

function CityPlanComponent:_create_fabricator(blueprint)
   local name = radiant.entities.get_name(blueprint)  
   local transform = blueprint:add_component('mob'):get_transform()
   local fabricator = radiant.entities.create_entity()
   
   fabricator:add_component('mob'):set_transform(transform)  
   fabricator:set_debug_text('fabricator for ' .. blueprint:get_debug_text())
   fabricator:add_component('stonehearth:fabricator')
                  :start_project(name, blueprint)
                  :set_debug_color(Color4(255, 0, 0, 128))
                  
   
   return fabricator
end

function CityPlanComponent:_create_project_recursive(blueprint)
   -- either you're a fabricator or you contain things which may be fabricators.  not
   -- both!
   local ec = blueprint:get_component('entity_container')
   local fabricator_info = radiant.entities.get_entity_data(blueprint, 'stonehearth:fabricator_info')  
   assert((not fabricator_info) or (not ec), 'blueprint man not have both fabricator data and children!')
   
   if fabricator_info then
      return self:_create_fabricator(blueprint)
   end
   return self:_create_sub_projects(blueprint)
end

function CityPlanComponent:_create_sub_projects(blueprint)
   local project_container = radiant.entities.create_entity(blueprint:get_uri())
   local transform = blueprint:add_component('mob'):get_transform()
   project_container:set_debug_text('project container for ' .. blueprint:get_debug_text())
   project_container:add_component('mob'):set_transform(transform)
   
   local ec = blueprint:get_component('entity_container')  
   if ec then
      for id, child in ec:get_children():items() do
         local project = self:_create_project_recursive(child)
         project_container:add_component('entity_container')
                              :add_child(project)
      end
   end
   return project_container
end

return CityPlanComponent
