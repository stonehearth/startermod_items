local Color4 = _radiant.csg.Color4
local CityPlanComponent = class()

function CityPlanComponent:__init(entity, data_binding)
   self._entity = entity
   self._data_binding = data_binding
   self._fabricators = {}
end

function CityPlanComponent:__tojson()
   local result = {
      fabricators = self._fabricators,
   }
   return radiant.json.encode(result)
end

function CityPlanComponent:extend(json)
end

function CityPlanComponent:add_blueprint(location, blueprint)
   -- create a new fabricator...   to... you know... fabricate
   local fabricator = radiant.entities.create_entity()
   local name = radiant.entities.get_name(blueprint)
   fabricator:set_debug_text('fabricator for ' .. blueprint:get_debug_text())
   fabricator:add_component('stonehearth:fabricator')
                  :start_project(name, location, blueprint)
                  :set_debug_color(Color4(255, 0, 0, 128))
   
   self._fabricators[fabricator:get_id()] = fabricator
end

return CityPlanComponent
