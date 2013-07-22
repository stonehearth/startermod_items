local CityPlanComponent = class()

function CityPlanComponent:__init(entity)
   self._entity = entity
   self._blueprints = {}
end

function CityPlanComponent:extend(json)
end

function CityPlanComponent:add_blueprint(entity)
   radiant.check.is_entity(entity)
   table.insert(self._blueprints, entity)
end

return CityPlanComponent
