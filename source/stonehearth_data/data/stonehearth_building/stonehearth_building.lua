local stonehearth_building = {
   STOREY_HEIGHT = 7
} 

local RadiantIPoint3 = _radiant.csg.Point3

function stonehearth_building.create_column(faction, x, z)
   local entity = radiant.entities.create_entity('/stonehearth_building/entities/column_blueprint')
   
   entity:add_component('unit_info'):set_faction(faction)
   entity:add_component('mob'):set_location_grid_aligned(RadiantIPoint3(x, 0, z));

   local region = native:alloc_region()
   entity:add_component('region_collision_shape'):set_region(region)
   
   entity:add_component('stonehearth_building:column_blueprint'):set_height(stonehearth_building.STOREY_HEIGHT)
   
   return entity
end

function stonehearth_building.create_wall(faction, p0, p1, normal)
   local entity = radiant.entities.create_entity('/stonehearth_building/entities/wall_blueprint')
   local unit_info = entity:add_component('unit_info')   
   local wall = entity:add_component('stonehearth_building:wall_blueprint')
   
   unit_info:set_faction(faction)
   wall:set_columns(p0, p1, normal)
   
   return entity
end

function stonehearth_building.create_room(faction, x, z, w, h)
   local building = stonehearth_building._create_new_building(faction, w, h)
   --radiant.terrain.place_entity(building, RadiantIPoint3(x, 1, z))
   
   return building
end

function stonehearth_building._create_structure_from_blueprint(entity, x, y, z)
   local blueprint_list = entity:get_component('stonehearth_building:blueprint_list')
   local structure = blueprint_list:create_structure()
   radiant.terrain.place_entity(structure, RadiantIPoint3(x, y, z))
end

function stonehearth_building._create_new_building(faction, w, h)
   assert(w > 2 and h > 2, string.format("room dimensions %d by %d is too small", w, h))
   
   -- create a new building and pull out the building component.
   local building_entity = radiant.entities.create_entity('/stonehearth_building/entities/building')
   building_entity:add_component('unit_info'):set_faction(faction)
   local building = building_entity:add_component('stonehearth_building:building')
   
   -- add the first storey to the building so we can start piling up walls and columns
   local storey_entity = radiant.entities.create_entity('/stonehearth_building/entities/storey')
   storey_entity:add_component('unit_info'):set_faction(faction)
   local storey = storey_entity:add_component('stonehearth_building:storey')
   building:add_storey(storey_entity)
   
   -- create the columns and walls...
   local columns = {
      stonehearth_building.create_column(faction, 0, 0),
      stonehearth_building.create_column(faction, w, 0),
      stonehearth_building.create_column(faction, w, h),
      stonehearth_building.create_column(faction, 0, h),
   }
   --[[
   local walls = {
      stonehearth_building.create_wall(faction, columns[1], columns[2], Point3(0, 0, -1)),
      stonehearth_building.create_wall(faction, columns[2], columns[3], Point3(1, 0, 0)),
      stonehearth_building.create_wall(faction, columns[3], columns[4], Point3(0, 0, 1)),
      stonehearth_building.create_wall(faction, columns[4], columns[1], Point3(-1, 0, 0)),     
   }
   ]]
   local storey_container = storey_entity:add_component('entity_container')
   for i = 1,4 do
      storey_container:add_child(columns[i])
      --storey_container:add_child(walls[i])
   end
   
   -- add the building plan to the root entity
   local root = radiant.entities.get_root_entity()
   local city_plan = root:add_component('stonehearth_building:city_plan')
   city_plan:add_blueprint(building_entity)
   
   return building_entity
end

return stonehearth_building
