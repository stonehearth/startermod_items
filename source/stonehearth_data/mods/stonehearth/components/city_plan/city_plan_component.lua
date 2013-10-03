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
   fabricator:add_component('stonehearth:fabricator'):start_project(location, blueprint)

   self._fabricators[fabricator:get_id()] = fabricator
end

return CityPlanComponent
