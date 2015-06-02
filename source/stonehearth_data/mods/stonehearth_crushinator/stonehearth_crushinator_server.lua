local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local WorkshopCallHandler = require 'stonehearth.call_handlers.workshop_call_handler'
local build_util = require 'stonehearth.lib.build_util'
local lrbt_util = require 'stonehearth_autotest.tests.longrunning.build.lrbt_util'

mod = {}


radiant.events.listen(mod, 'radiant:new_game', function(args)
	autotest_framework.env.set_world_generator_script('/stonehearth_crushinator/large_world.lua')
	autotest_framework.env.create_world()
	radiant.set_realtime_timer("crushinator server crushinator_tests_maul", 3000, crushinator_tests_maul)
end)

function maintain_stock(workshop, recipe_uri)
   local condition = {
      at_least = 2,
   }
   condition['type'] = 'maintain'

   workshop:get_component('stonehearth:workshop'):add_order({player_id='player_1'}, nil, recipe_uri, condition)
end

function workshop_init(worker, workshop_uri, shop_x, shop_y, stock)
   WorkshopCallHandler():create_ghost_workbench({player_id='player_1'}, nil, workshop_uri, Point3(shop_x, 11, shop_y), 0, worker:get_id())   

   local workshop
   radiant.events.listen_once(worker, 'stonehearth:crafter:workshop_changed', function (e)
         workshop = e.workshop:get_entity()

         for _, s in ipairs(stock) do
            local recipe = radiant.resources.load_json(s)
            maintain_stock(workshop, recipe)
         end
      end)
end

function mason_init(mason, shop_x, shop_y)
   local recipes = {
      "file(stonehearth/jobs/mason/recipes/stone_chair_recipe.json)", 
      "file(stonehearth/jobs/mason/recipes/stone_bench_recipe.json)", 
      "file(stonehearth/jobs/mason/recipes/stone_bricks_recipe.json)", 
   }
   workshop_init(mason, 'stonehearth:mason:workbench', shop_x, shop_y, recipes)
end

function blacksmith_init(blacksmith, shop_x, shop_y)
   local recipes = {
      "file(stonehearth/jobs/blacksmith/recipes/bronze_ingot_recipe.json)", 
      "file(stonehearth/jobs/blacksmith/recipes/copper_ingot_recipe.json)", 
      "file(stonehearth/jobs/blacksmith/recipes/gold_ingot_recipe.json)", 
      "file(stonehearth/jobs/blacksmith/recipes/silver_ingot_recipe.json)", 
      "file(stonehearth/jobs/blacksmith/recipes/iron_ingot_recipe.json)", 
      "file(stonehearth/jobs/blacksmith/recipes/steel_ingot_recipe.json)", 
      "file(stonehearth/jobs/blacksmith/recipes/tin_ingot_recipe.json)", 
   }
   workshop_init(blacksmith, 'stonehearth:blacksmith:workbench', shop_x, shop_y, recipes)
end

function weaver_init(weaver, shop_x, shop_y)
   local recipes = {
      "file(stonehearth/jobs/weaver/recipes/cloth_bolt_recipe.json)", 
      "file(stonehearth/jobs/weaver/recipes/leather_bolt_recipe.json)", 
      "file(stonehearth/jobs/weaver/recipes/thread_recipe.json)", 
   }
   workshop_init(weaver, 'stonehearth:weaver:workbench', shop_x, shop_y, recipes)
end

function carpenter_init(carpenter, shop_x, shop_y)
   local recipes = {
      "file(stonehearth/jobs/carpenter/recipes/not_much_of_a_bed_recipe.json)", 
      "file(stonehearth/jobs/carpenter/recipes/table_for_one_recipe.json)", 
      "file(stonehearth/jobs/carpenter/recipes/simple_wooden_chair_recipe.json)", 
      "file(stonehearth/jobs/carpenter/recipes/wooden_door_recipe.json)", 
      "file(stonehearth/jobs/carpenter/recipes/wooden_door_2_recipe.json)", 
      "file(stonehearth/jobs/carpenter/recipes/wooden_double_door_recipe.json)", 
      "file(stonehearth/jobs/carpenter/recipes/dining_table_recipe.json)", 
      "file(stonehearth/jobs/carpenter/recipes/wooden_window_frame_recipe.json)", 
      "file(stonehearth/jobs/carpenter/recipes/wooden_wall_lantern_recipe.json)", 
   }
   workshop_init(carpenter, 'stonehearth:carpenter:workbench', shop_x, shop_y, recipes)
end

function new_trapping_grounds(x, y, width, height)
   autotest_framework.env.create_trapping_grounds(x, y, { size = { x = width, z = height }})
end

function create_wooden_floor(session, cube)
   return stonehearth.build:add_floor(session, WOODEN_FLOOR, cube)
end

function new_template(template_name, x, y)
   local session = {
      player_id = 'player_1',
   }
	local t = stonehearth.build:build_template_command(session, nil, template_name, Point3(x, 10, y), Point3(0, 10, 0), 0).building
   stonehearth.build:set_active(t, true)
   return t
end

function new_house(x, y)
   local session = {
      player_id = 'player_1',
   }

   local house
   stonehearth.build:do_command('house', nil, function()
         local w, h = 3, 3
         local floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(x, 10, y), Point3(x + w, 11, y + h)))
         house = build_util.get_building_for(floor)
         local walls = lrbt_util.grow_wooden_walls(session, house)
         lrbt_util.grow_wooden_roof(session, walls)
      end)
   stonehearth.build:set_active(house, true)
   return house
end

function new_shepherd_pasture(x, z, x_size, z_size)
	return autotest_framework.env.create_shepherd_pasture(x, z, x_size, z_size)
end

function new_farm(crop_x, crop_y, size_x, size_y, crop_type)
   autotest_framework.env.create_farm(crop_x, crop_y, { size = { x = size_x, y = size_y }, crop = crop_type})
end

function new_mine(min_point, max_point)
   local region = Region3(Cube3(min_point, max_point))

   stonehearth.mining:dig_region('player_1', region)
end

function new_road(min_point, max_point)
   local session = {
      player_id = 'player_1',
   }
   local road
   stonehearth.build:do_command('add_road', nil, function()
      road = stonehearth.build:add_road(session, 'stonehearth:build:brushes:pattern:wood_dark_diagonal', Cube3(Point3(min_point.x, 9, min_point.y), Point3(max_point.x, 10, max_point.y)))
   end)
   return road
end

function crushinator_tests_maul()
   --[[local CAMERA_POSITION = Point3(100, 100, 100)
   local CAMERA_LOOK_AT = Point3(0, 0, 0)
   autotest_framework.ui.move_camera(CAMERA_POSITION, CAMERA_LOOK_AT)]]

   local stockpile = autotest_framework.env.create_stockpile(0, -15, { size = { x = 9, y = 9 }})
   local stockpile = autotest_framework.env.create_stockpile(10, -15, { size = { x = 9, y = 9 }})
   local stockpile = autotest_framework.env.create_stockpile(0, -5, { size = { x = 9, y = 9 }})
   local stockpile = autotest_framework.env.create_stockpile(10, -5, { size = { x = 9, y = 9 }})

   local session = {
      player_id = 'player_1',
   }

   stonehearth.farming:add_crop_type(session, 'stonehearth:crops:tester_crop_2')
   stonehearth.farming:add_crop_type(session, 'stonehearth:crops:tester_silkweed_crop')

   new_mine(Point3(-60, 0, 40), Point3(-40, 10, 60))

   new_shepherd_pasture(90, 20, 30, 30)

   autotest_framework.env.create_entity_cluster(8, 0, 5, 5, 'stonehearth:resources:fiber:silkweed_bundle')
   autotest_framework.env.create_entity_cluster(8, 8, 7, 7, 'stonehearth:resources:wood:oak_log')
   autotest_framework.env.create_entity_cluster(16, 8, 7, 7, 'stonehearth:resources:wood:oak_log')
   autotest_framework.env.create_entity_cluster(-8, 8, 7, 7, 'stonehearth:resources:stone:hunk_of_stone')
   autotest_framework.env.create_entity_cluster(-8, -8, 2, 2, 'stonehearth:food:berries:berry_basket')


   new_template('tiny cottage', 50, 46)
   new_template('tiny cottage', 30, 46)
   new_template('tiny cottage', 10, 46)
   new_template('tiny cottage', 60, 16)
   new_template('tiny cottage', 40, 16)


   --[[new_house(10, 20)
   new_house(50, -20)
   new_house(30, -20)
   new_house(10, -20)
   new_house(-50, -20)
   new_house(-30, -20)
   new_house(-10, -20)]]
   --new_template(autotest, 50, 50, "Tiny Cottage")
   --new_template(autotest, 10, 50, "Shared Sleeping Quarters")
   --new_template(autotest, 50, 20, "Dining Hall")

   local road
   road = new_road(Point2(30, 33), Point2(70, 35))
   road = new_road(Point2(70, 5), Point2(73, 35))
   road = new_road(Point2(39, 5), Point2(73, 8))
   stonehearth.build:set_active(build_util.get_building_for(road), true)

   road = new_road(Point2(0, 57), Point2(70, 60))
   stonehearth.build:set_active(build_util.get_building_for(road), true)
   

   -- 25 workers
   for i = -4, 4, 2 do
      for j = -4, 4, 2 do
         autotest_framework.env.create_person(i, j, { job = 'worker' })
      end
   end
   -- 2 farmers
   autotest_framework.env.create_person(6, -2, { job = 'farmer' })
   autotest_framework.env.create_person(6, 0, { job = 'farmer' })

   new_farm(-65, -10, 40, 15, 'stonehearth:crops:tester_crop_2')
   new_farm(80, -10, 40, 15, 'stonehearth:crops:tester_silkweed_crop')
   -- 2 carpenters
   local carp_1 = autotest_framework.env.create_person(6, -6, { job = 'carpenter', attributes = { total_level = 3 } })
   local carp_2 = autotest_framework.env.create_person(6, -4, { job = 'carpenter' })


   carpenter_init(carp_1, -10, -10)
   carpenter_init(carp_2, 20, -10)

   -- 2 weavers
   local weav_1 = autotest_framework.env.create_person(6, 2, { job = 'weaver' })
   local weav_2 = autotest_framework.env.create_person(6, 4, { job = 'weaver' })


   new_trapping_grounds(-120, -100, 80, 60)
   new_trapping_grounds(40, -100, 80, 60)


   weaver_init(weav_1, -10, 0)
   weaver_init(weav_2, -10, 10)

   -- 2 blacksmiths
   local bs_1 = autotest_framework.env.create_person(6, 6, { job = 'blacksmith' })
   local bs_2 = autotest_framework.env.create_person(8, -6, { job = 'blacksmith' })

   blacksmith_init(bs_1, -20, 0)
   blacksmith_init(bs_2, -20, 10)

   -- 2 masons
   local mas_1 = autotest_framework.env.create_person(8, 0, { job = 'mason' })
   local mas_2 = autotest_framework.env.create_person(8, 2, { job = 'mason' })
   
   mason_init(mas_2, -30, 10)
   mason_init(mas_1, -30, 20)

   -- 2 trappers
   local tr_1 = autotest_framework.env.create_person(8, -4, { job = 'trapper' })
   local tr_2 = autotest_framework.env.create_person(8, -2, { job = 'trapper' })

   -- 2 shepherds
   local tr_1 = autotest_framework.env.create_person(10, -4, { job = 'shepherd' })
   local tr_2 = autotest_framework.env.create_person(10, -2, { job = 'shepherd' })
end

return mod
