local CritterNest = class()

function CritterNest:__init()
end

function CritterNest:initialize(properties, services)
   local info = properties.scenario_info
   local num_critters = services.rng:get_int(info.num_critters.min, info.num_critters.max)
   services:place_entity_cluster(info.critter_type, num_critters, info.critter_length)
end

return CritterNest
