local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local build_util = require 'stonehearth.lib.build_util'
local lrbt_util = require 'tests.longrunning.build.lrbt_util'

local crushinator_tests = {}

function maintain_stock(autotest, recipe_name)
   -- Assumes the crafting window is open!

   autotest.ui:click_dom_element('#craftWindow #recipeList div[title="' .. recipe_name .. '"]')
   autotest.ui:click_dom_element('#craftWindow #maintain')
   autotest.ui:click_dom_element('#craftWindow #craftButton')
end

function workshop_init(autotest, worker, shop_x, shop_y, stock)
   autotest.ui:push_unitframe_command_button(worker, 'build_workshop')
   autotest.ui:click_terrain(shop_x, shop_y)

   local workshop
   radiant.events.listen_once(worker, 'stonehearth:crafter:workshop_changed', function (e)
         workshop = e.workshop:get_entity()
         autotest.ui:push_unitframe_command_button(workshop, 'show_workshop')

         for _, s in pairs(stock) do
            maintain_stock(autotest, s)
         end

         autotest.ui:click_dom_element('#craftWindow #closeButton')
         autotest:resume()
      end)

   autotest:suspend()
end

function mason_init(autotest, mason, shop_x, shop_y)
   workshop_init(autotest, mason, shop_x, shop_y, {"Stone Chair", "Stone Bench", "Sack of Stone Bricks"})
end

function blacksmith_init(autotest, blacksmith, shop_x, shop_y)
   workshop_init(autotest, blacksmith, shop_x, shop_y, {"Bronze Ingot", "Copper Ingot", "Gold Ingot", "Silver Ingot", "Iron Ingot", "Steel Ingot", "Tin Ingot"})
end

function weaver_init(autotest, weaver, shop_x, shop_y)
   workshop_init(autotest, weaver, shop_x, shop_y, {"Bolt of Cloth", "Bundle of Leather", "Spool of Thread"})
end

function carpenter_init(autotest, carpenter, shop_x, shop_y)
   workshop_init(autotest, carpenter, shop_x, shop_y, {"Mean Bed", "Table for One", "Simple Wooden Chair", "Wooden Door", "Reinforced Wooden Door", "Dining Table", "Wooden Window Frame", "Wooden Wall-mounted Lantern"})
end

function new_trapping_grounds(autotest, x, y, width, height)
   autotest.env:create_trapping_grounds(x, y, { size = { x = width, z = height }})
end

function new_house(x, y)
   local session = {
      player_id = 'player_1',
   }

   local house
   stonehearth.build:do_command('house', nil, function()
         local w, h = 3, 3
         local floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(x, 9, y), Point3(x + w, 10, y + h)))
         house = build_util.get_building_for(floor)
         lrbt_util.grow_wooden_walls(session, house)
         lrbt_util.grow_wooden_roof(session, house)
      end)
   stonehearth.build:set_active(house, true)
   return house
end


function new_template(autotest, x, y, template_name)
   autotest.ui:click_dom_element('#build_menu')
   autotest.ui:click_dom_element('#building_templates')
   autotest.ui:click_dom_element('#templatesList [template="' .. template_name .. '"]')
   
   autotest.ui:click_terrain(x, y)
   autotest.ui:click_terrain(x, y)
   autotest.ui:click_dom_element('#startBuilding')
   autotest.ui:click_dom_element('#confirmBuilding')
   autotest.ui:click_dom_element('#startMenu .close')
end

function new_farm(autotest, crop_x, crop_y, size_x, size_y, crop_type)
   autotest.ui:click_dom_element('#zone_menu')
   autotest.ui:click_dom_element('#create_farm')
   autotest.ui:set_next_designation_region(crop_x, crop_y, size_x, size_y)

   autotest.ui:click_dom_element('#farmWindow #addCropLink')
   autotest.ui:click_dom_element('[crop="stonehearth:crops:' .. crop_type .. '"]')

   autotest.ui:click_dom_element('#farmWindow .ok')

   autotest.ui:click_dom_element('.stonehearthMenu .close')
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
      road = stonehearth.build:add_road(session, 'stonehearth:build:brushes:pattern:wood_dark_diagonal', nil, Cube3(Point3(min_point.x, 9, min_point.y), Point3(max_point.x, 10, max_point.y)))
   end)
   return road
end

function crushinator_tests.maul(autotest)
   local CAMERA_POSITION = Point3(100, 100, 100)
   local CAMERA_LOOK_AT = Point3(0, 0, 0)
   autotest_framework.ui.move_camera(CAMERA_POSITION, CAMERA_LOOK_AT)

   local stockpile = autotest.env:create_stockpile(-5, -20, { size = { x = 9, y = 9 }})
   local stockpile = autotest.env:create_stockpile( 5, -20, { size = { x = 9, y = 9 }})
   local stockpile = autotest.env:create_stockpile(-5, -10, { size = { x = 9, y = 9 }})
   local stockpile = autotest.env:create_stockpile( 5, -10, { size = { x = 9, y = 9 }})

   local session = {
      player_id = 'player_1',
   }

   stonehearth.farming:add_crop_type(session, 'stonehearth:crops:tester_crop_2')
   stonehearth.farming:add_crop_type(session, 'stonehearth:crops:tester_silkweed_crop')

   new_mine(Point3(-60, -16, 40), Point3(-40, 10, 60))

   autotest.env:create_entity_cluster(8, 0, 5, 5, 'stonehearth:resources:fiber:silkweed_bundle')
   autotest.env:create_entity_cluster(8, 8, 7, 7, 'stonehearth:resources:wood:oak_log')
   autotest.env:create_entity_cluster(-8, 8, 7, 7, 'stonehearth:resources:stone:hunk_of_stone')
   autotest.env:create_entity_cluster(-8, -8, 2, 2, 'stonehearth:food:berries:berry_basket')

   new_house(50, 50)
   new_house(30, 50)
   new_house(10, 50)
   new_house(50, 20)
   --new_template(autotest, 50, 50, "Tiny Cottage")
   --new_template(autotest, 10, 50, "Shared Sleeping Quarters")
   --new_template(autotest, 50, 20, "Dining Hall")

   local road
   road = new_road(Point2(0, 33), Point2(70, 35))
   road = new_road(Point2(70, 5), Point2(73, 35))
   road = new_road(Point2(39, 5), Point2(73, 8))
   stonehearth.build:set_active(build_util.get_building_for(road), true)

   road = new_road(Point2(0, 57), Point2(70, 60))
   stonehearth.build:set_active(build_util.get_building_for(road), true)
   --autotest:sleep(15 * 1000)
   --new_template(autotest, -40, 50, "Cottage for Two")


   -- 25 workers
   for i = -4, 4, 2 do
      for j = -4, 4, 2 do
         autotest.env:create_person(i, j, { job = 'worker' })
      end
   end
--[[
   -- 2 carpenters
   local carp_1 = autotest.env:create_person(6, -6, { job = 'carpenter', attributes = { total_level = 3 } })
   local carp_2 = autotest.env:create_person(6, -4, { job = 'carpenter' })

   -- 2 farmers
   autotest.env:create_person(6, -2, { job = 'farmer' })
   autotest.env:create_person(6, 0, { job = 'farmer' })

   -- 2 weavers
   local weav_1 = autotest.env:create_person(6, 2, { job = 'weaver' })
   local weav_2 = autotest.env:create_person(6, 4, { job = 'weaver' })

   new_farm(autotest, -40, -10, 10, 10, 'tester_crop_2')
   new_farm(autotest, 40, -10, 20, 10, 'tester_silkweed_crop')

   new_trapping_grounds(autotest, -80, -40, 20, 20)
   new_trapping_grounds(autotest, 80, -40, 20, 20)

   carpenter_init(autotest, carp_1, -10, -10)
   carpenter_init(autotest, carp_2, 20, -10)

   weaver_init(autotest, weav_1, -10, 0)
   weaver_init(autotest, weav_2, -10, 10)

   -- 2 blacksmiths
   local bs_1 = autotest.env:create_person(6, 6, { job = 'blacksmith' })
   local bs_2 = autotest.env:create_person(8, -6, { job = 'blacksmith' })

   blacksmith_init(autotest, bs_1, -20, 0)
   blacksmith_init(autotest, bs_2, -20, 10)

   -- 2 masons
   local mas_1 = autotest.env:create_person(8, 0, { job = 'mason' })
   local mas_2 = autotest.env:create_person(8, 2, { job = 'mason' })
   
   mason_init(autotest, mas_1, -20, 0)
   mason_init(autotest, mas_2, -20, 10)

   -- 2 trappers
   local tr_1 = autotest.env:create_person(8, -4, { job = 'trapper' })
   local tr_2 = autotest.env:create_person(8, -2, { job = 'trapper' })
]]
   autotest:sleep(1 * 60 * 60 * 1000)
end

return crushinator_tests

