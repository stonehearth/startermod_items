local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local AlignmentTest = class(MicroWorld)

function AlignmentTest:__init()
   self[MicroWorld]:__init(256)
   self:create_world()

   --self:place_citizen(0, 0, 'trapper')
   --self:place_citizen(2, 2)

   local col = { -120, -40, -8, 10 }
   local row = { -120 }
   local x, z, dx, dz, x_start, z_start

   x_start, z_start = col[1], row[1]
   x,z = x_start,z_start; dx,dz = 20,20
   self:_check_item('stonehearth:large_oak_tree', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:medium_oak_tree', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:small_oak_tree', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:large_juniper_tree', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:medium_juniper_tree', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:small_juniper_tree', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:large_boulder', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:medium_boulder', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:small_boulder', x, z, dx, 0); x=x_start; z=z+dz;

   x_start, z_start = col[2], row[1]
   x,z = x_start,z_start; dx,dz = 8,8
   self:_check_item('stonehearth:berry_hanging', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:geranium_hanging', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:green_hanging', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:camp_standard', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:tombstone', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:not_much_of_a_bed', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:comfy_bed', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:arch_backed_chair', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:simple_wooden_chair', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:dining_table', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:table_for_one', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:wooden_wall_lantern', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:wooden_garden_lantern', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:picket_fence', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:firepit', x, z, dx, 0); x=x_start; z=z+dz;

   x_start, z_start = col[3], row[1]
   x,z = x_start,z_start; dx,dz = 4,8
   self:_check_item('stonehearth:berry_hanging', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:geranium_hanging', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:green_hanging', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:camp_standard', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:tombstone', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:not_much_of_a_bed', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:comfy_bed', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:arch_backed_chair', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:simple_wooden_chair', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:dining_table', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:table_for_one', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:wooden_wall_lantern', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:wooden_garden_lantern', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:picket_fence', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:firepit', x, z, dx, 0); x=x_start; z=z+dz;

--

   self:_check_item('stonehearth:picket_fence_gate', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:wooden_door', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:wooden_window_frame', x, z, dx, 0); x=x_start; z=z+dz;


   x_start, z_start = col[4], row[1]
   x,z = x_start,z_start; dx,dz = 4,4
   self:_check_item('stonehearth:default_object', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:oak_log', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:juniper_log', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:wool_bundle', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:cloth_bolt', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:mixins:resources:fur_pelt', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:amazing_corn_basket', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:corn_basket', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:turnip_basket', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:berry_basket', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:berry_plate', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:berry_bush', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:pink_flower', x, z, dx, 0); x=x_start; z=z+dz;
   self:_check_item('stonehearth:terrain:tall_grass', x, z, dx, 0); x=x_start; z=z+dz;

   self:_check_item('stonehearth:ball', x, z, dx, 0); x=x_start; z=z+dz;

end

function AlignmentTest:_check_item(uri, x, z, dx, dz)
   for angle = 0, 270, 90 do
      local item = self:_place_item(uri, x, z, angle)
      local collision_component = item:get_component('region_collision_shape')
      if collision_component then
         self:_place_mark(x, z)
      end
      x = x + dx
      z = z + dz
   end
end

function AlignmentTest:_place_item(uri, x, z, angle)
   local item = self:place_item(uri, x, z)
   item:add_component('mob'):turn_to(angle)
   return item
end

function AlignmentTest:_place_mark(x, z)
   --self:place_citizen(x, z, 'trapper')
   self:place_item('stonehearth:default_object', x, z)
   --local proxy_entity = radiant.entities.create_proxy_entity(false)
   --radiant.terrain.place_entity(proxy_entity, Point3(x, 0, z))
end

return AlignmentTest
