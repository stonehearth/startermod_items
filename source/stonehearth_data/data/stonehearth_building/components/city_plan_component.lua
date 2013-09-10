local CityPlanComponent = class()

function CityPlanComponent:__init(entity)
   self._entity = entity
   self._blueprints = {}
end

function CityPlanComponent:__tojson()
   local result = {
      blueprints = self._blueprints,
   }
   return radiant.json.encode(result)
end

function CityPlanComponent:extend(json)
end

function CityPlanComponent:add_blueprint(entity)
   self._blueprints[entity:get_id()] = entity
end

return CityPlanComponent
