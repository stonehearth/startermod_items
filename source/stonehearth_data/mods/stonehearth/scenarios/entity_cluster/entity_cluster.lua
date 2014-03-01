local EntityCluster = class()

function EntityCluster:__init()
end

function EntityCluster:initialize(properties, services)
   local info = properties.scenario_info
   local num_entities = services.rng:get_int(info.quantity.min, info.quantity.max)
   services:place_entity_cluster(info.entity_type, num_entities, info.entity_footprint_size)
end

return EntityCluster
