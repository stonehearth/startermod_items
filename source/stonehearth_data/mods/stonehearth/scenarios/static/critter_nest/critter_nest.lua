local CritterNest = class()

function CritterNest:__init()
end

function CritterNest:initialize(properties, services)
   local data = properties.data
   local num_critters = services.rng:get_int(data.num_critters.min, data.num_critters.max)
   services:place_entity_cluster(data.critter_type, num_critters, data.critter_length)
end

return CritterNest
