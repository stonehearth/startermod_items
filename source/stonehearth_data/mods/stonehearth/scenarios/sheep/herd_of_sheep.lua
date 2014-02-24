local HerdOfSheep = class()

function HerdOfSheep:__init()
end

function HerdOfSheep:initialize(properties, services)
   local sheep_name = 'stonehearth:sheep'
   local num_sheep = services.rng:get_int(1, 5)
   local sheep_radius = 1
   services:place_entity_cluster(sheep_name, num_sheep, sheep_radius)
end

return HerdOfSheep
