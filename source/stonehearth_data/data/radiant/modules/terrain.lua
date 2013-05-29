local Terrain = {}

local _terrain = radiant._root_entity:add_terrain_component()

function Terrain.add_cube(cube)
   _terrain:add_cube(cube)
end

function Terrain.place_entity(entity, location)
   radiant.check.is_a(location, RadiantIPoint3)

   radiant.entities.add_child(radiant._root_entity, entity, location)
   radiant.components.add_component(entity, 'render_info'):set_display_iconic(true);
   _terrain:place_entity(entity, location)
end

return Terrain
