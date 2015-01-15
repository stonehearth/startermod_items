local EntityCluster = class()

function EntityCluster:__init()
end

function EntityCluster:initialize(properties, services)
   local data = properties.data
   local num_entities = services.rng:get_int(data.quantity.min, data.quantity.max)
   services:place_entity_cluster(data.entity_type, num_entities, data.entity_footprint_length)
end

return EntityCluster
