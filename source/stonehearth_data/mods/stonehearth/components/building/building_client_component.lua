local BuildingClientComponent = class()

function BuildingClientComponent:initialize(entity, json)
   self._entity = entity
end

function BuildingClientComponent:destroy()
end

function BuildingClientComponent:load_from_template(template, options, entity_map)
   -- create a fake envelop here
end

return BuildingClientComponent
