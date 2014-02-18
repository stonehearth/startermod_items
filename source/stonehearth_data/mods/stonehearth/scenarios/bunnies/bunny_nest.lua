local BunnyNest = class()

function BunnyNest:__init()
end

function BunnyNest:initialize(properties, services)
   local rabbit_name = 'stonehearth:rabbit'
   local num_rabbits = services.rng:get_int(2, 6)
   local rabbit_radius = 0
   services:place_entity_cluster(rabbit_name, num_rabbits, rabbit_radius)
end

return BunnyNest
