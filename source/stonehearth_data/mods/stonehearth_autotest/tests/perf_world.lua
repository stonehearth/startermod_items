
function create_world(environment)
   local wgs = stonehearth.world_generation
   wgs:create_new_game(12345, false)
   local blueprint = wgs.blueprint_generator:generate_blueprint(12,8, 12345)
   wgs:set_blueprint(blueprint)
   wgs:generate_tiles(6, 4, 2)
end

return create_world