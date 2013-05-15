local GameMaster = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local check = require 'radiant.core.check'
local tg = require 'radiant.core.tg'
local sh = require 'radiant.core.sh'
local log = require 'radiant.core.log'
local ai_mgr = require 'radiant.ai.ai_mgr'

Scenario = {
   RunLevel = {     
      BACKGROUND = 1,
      ACTIVE = 2,
   }
}

function GameMaster:__init()
   self._registered_scenarios = {}
   self._scenarios = {}
   
   md:register_msg_handler_instance('radiant.gm', self)
   md:register_msg('radiant.scenario.set_run_level')
end

function GameMaster:register_scenario(name, ctor)
   check:verify(not self._registered_scenarios[name])
   self._registered_scenarios[name] = true
   md:register_msg_handler(name, ctor)
end

function GameMaster:start_scenario(name, box)
   check:verify(self._registered_scenarios[name])
   local options = {
      box = box,
   }
   return md:create_msg_handler(name, options)
end

function create_stairs(source, direction, width, depth)
   -- cut out the tops   
   local cutter
   if direction.x == 0 then
      cutter = Cube3(Point3(source.x - width / 2, source.y, source.z),
                     Point3(source.x + width / 2, 50, source.z + 1));
   else
      cutter = Cube3(Point3(source.x, source.y, source.z - width / 2),
                     Point3(source.x + 1, 50, source.z + width / 2));
   end
   local terrain = om:get_terrain()
   for i = 0, depth-1 do
      terrain:remove_cube(cutter)
      terrain:add_cube(
         Cube3(cutter.min, Point3(cutter.max.x, cutter.min.y + 1, cutter.max.z), Terrain.DARK_WOOD)
      )
      cutter = Cube3(Point3(cutter.min.x + direction.x, cutter.min.y + 1, cutter.min.z + direction.z),
                     Point3(cutter.max.x + direction.x, cutter.max.y + 1, cutter.max.z + direction.z));
   end
end

function create_road(from, to, height)
   local terrain = om:get_terrain()

   local w = 2
   local cutter
   if from.x == to.x then
      cutter = Cube3(Point3(from.x - w, height - 1, from.y - w),
                     Point3(to.x + w,   height,     to.y + w),
                     Terrain.DIRT_PATH)
   else
      cutter = Cube3(Point3(from.x - w,   height - 1, from.y - w),
                     Point3(to.x + w, height,     to.y + w),
                     Terrain.DIRT_PATH)
   end

   terrain:remove_cube(cutter)
   terrain:add_cube(cutter)

end

local ch = require 'radiant.core.ch'

function create_room(x, y, z, w, h)
   local bounds = RadiantBounds3(RadiantIPoint3(x, y, z),
                                 RadiantIPoint3(x + w, y + 1, z + h))
   local json, obj = ch:call('radiant.commands.create_room', bounds)
   return om:get_entity(obj.entity_id)
end

function create_door(wall, x, y, z)
   check:is_a(wall, Wall)
   local json, obj = ch:call('radiant.commands.create_portal', wall, 'module://stonehearth/buildings/wooden_door', RadiantIPoint3(x, y, z))
   return om:get_entity(obj.entity_id)
end

function add_door_to_wall(room, wall_normal, offset)
   local ec = om:get_component(room, 'entity_container')
   ec:iterate_children(function(id, child)
      if om:has_component(child, 'wall') then
         local wall = om:get_component(child, 'wall')
         local normal = wall:get_normal()
         if normal.x == wall_normal.x and normal.z == wall_normal.z then
            local xo = wall_normal.x == 0 and offset or 0
            local zo = wall_normal.z == 0 and offset or 0
            create_door(wall, xo, 0, zo)
         end
      end
   end)
end

function create_stockpile(loc, w, h)
   local bounds = RadiantBounds3(RadiantIPoint3(loc.x, loc.y, loc.z),
                                 RadiantIPoint3(loc.x + w, loc.y + 1, loc.z + h))
   return ch:call('radiant.commands.create_stockpile', bounds)
end

local YZ = Point2(213, 256)
local YZ2 = Point2(213, 260)
local ZA = Point2(213, 202)
local AB = Point2(226, 202)
local BC = Point2(226, 195)
local CD = Point2(235, 195)
local DE = Point2(245, 195)
local EH = Point2(261, 195)
local HI = Point2(261, 176)
local FI = Point2(235, 176)
local GK = Point2(245, 247)
local GK2 = Point2(245, 251)
local K = Point2(245, 259)
local L = Point2(345, 259)
local MN = Point2(245, 282)
local NY = Point2(213, 282)
local TT = Point2(301, 259)

-- construction
function scene_1()
   create_stairs(Point3(216, 1, 188), Point3(-1, 0, 0), 4, 3)

   create_stockpile(Point3(ZA.x + 3, 1, ZA.y + 3), 5, 5);
   create_stockpile(Point3(BC.x, 1, BC.y - 7), 5, 5);
   
   sh:create_citizen(RadiantIPoint3(ZA.x + 2, 1, ZA.y))
   sh:create_citizen(RadiantIPoint3(ZA.x + 16, 1, ZA.y + 9))
   sh:create_citizen(RadiantIPoint3(ZA.x + 22, 1, ZA.y + 13))
   
   for i = 0, 25 do
      local x = math.random(100, 300)
      local z = math.random(100, 300)
      local rabbit = om:create_entity('radiant.mobs.rabbit')
      om:place_on_terrain(rabbit, RadiantIPoint3(x, 1, z))      
   end
end

function scene_2()
   sh:create_citizen(RadiantIPoint3(ZA.x + 2, 1, ZA.y + 5))
   sh:create_citizen(RadiantIPoint3(ZA.x + 16, 1, ZA.y + 15))
   sh:create_citizen(RadiantIPoint3(ZA.x + 22, 1, ZA.y + 25))
   scene_1()
end

function start_project_cmd(blueprint)
   check:is_entity(blueprint);
   local json, obj = ch:call('radiant.commands.start_project', blueprint)
   return om:get_entity(obj.entity_id)
end

local room_a, room_b, room_c, room_d, room_x, room_y, room_z
function complete_to(room, items) 
   local complete_build_order = function(id, entity)
      for name, percent in pairs(items) do
         local bo = om:get_component(entity, name)
         if bo and bo.complete_to then
            bo:complete_to(percent)
         end
      end
   end
   local ec = om:get_component(room, 'entity_container')
   ec:iterate_children(complete_build_order)
end

function scene_3()
   scene_2()
   local w, h, padding = 10, 11, 3
   room_a = create_room(GK.x - padding - w, 1, GK.y - padding - h, w, h)      
   room_b = create_room(GK.x - padding - w, 1, GK.y - (padding + h) * 2 - padding, w, h)
   
   add_door_to_wall(room_a, Point3(1, 0, 0), 4)
   add_door_to_wall(room_b, Point3(1, 0, 0), 4)
      
   room_b = start_project_cmd(room_b)
   complete_to(room_b, { post = 100 })
   
   create_stairs(Point3(245, 1, 247), Point3(0, 0, 1), 4, 3)
end

function scene_4()
   scene_3()

   local w, h, padding = 10, 7, 3
   room_c = create_room(MN.x - padding - w, 4, MN.y - padding - h, w, h)      
   room_d = create_room(MN.x - padding - w, 4, MN.y - (padding + h) * 2 - padding, w, h)
   
   w, h, padding = 13, 17, 3
   room_x = create_room(NY.x + padding, 4, NY.y - padding - h, w, h)      
   
   w, h, padding = 9, 10, 4
   room_y = create_room(YZ.x + padding, 1, YZ.y - padding - h, w, h)
   
   add_door_to_wall(room_c, Point3(1, 0, 0), 2)
   add_door_to_wall(room_d, Point3(1, 0, 0), 2)
   add_door_to_wall(room_x, Point3(0, 0, -1), 3)
   add_door_to_wall(room_y, Point3(-1, 0, 0), 3)
   
   room_a = start_project_cmd(room_a)
   room_c = start_project_cmd(room_c)
   room_d = start_project_cmd(room_d)
   room_x = start_project_cmd(room_x)
   room_y = start_project_cmd(room_y)
   
   local finished = {
      post = 100,
      wall = 100,
      peaked_roof = 100,
      fixture = 100
   }
   complete_to(room_a, finished)
   complete_to(room_b, finished)   
   complete_to(room_c, finished)   
   complete_to(room_d, finished)
   complete_to(room_x, finished)
   complete_to(room_y, finished)
   
   local dx, dy = 8, 12
   local tower = om:create_entity('radiant.structures.tower')
   om:place_on_terrain(tower, RadiantIPoint3(EH.x - dx, 1, EH.y - dy))

   --local tower = om:create_entity('radiant.structures.tower')
   --om:place_on_terrain(tower, RadiantIPoint3(TT.x, 1, TT.y + 6))
   local tower = om:create_entity('radiant.structures.tower')
   om:place_on_terrain(tower, RadiantIPoint3(TT.x, 1, TT.y - 18))

   create_stairs(Point3(YZ.x, 1, YZ.y), Point3(0, 0, 1), 4, 3)   
   create_road(ZA, YZ, 1); -- Z
   create_road(ZA, AB, 1); -- A
   create_road(BC, AB, 1); -- B
   create_road(BC, CD, 1); -- C
   create_road(CD, DE, 1); -- D
   create_road(DE, EH, 1); -- E
   create_road(HI, EH, 1); -- H
   create_road(FI, HI, 1); -- I
   create_road(FI, CD, 1); -- F   
   create_road(DE, GK, 1); -- G
   
   create_road(GK2, K, 4); -- K snippet
   create_road(K, L, 4); -- L
   create_road(K, MN, 4); -- M
   create_road(NY, MN, 4); -- N
   create_road(YZ2, NY, 4); -- N
end

-- running from the woods
function scene_5()
   scene_4()
   local goblins = {}
   local soldiers = {}
   local goblin_weapons = {
      'module://stonehearth/items/spiked_wooden_mace'
   }
   local spawn_goblin = function(x, z)
      local goblin = om:create_entity('module://stonehearth/mobs/goblin_soldier')
      local shield = om:create_entity('module://stonehearth/items/cracked_wooden_buckler')
      local helmet = om:create_entity('module://stonehearth/items/studded_leather_helmet')
      local mace = om:create_entity(goblin_weapons[math.random(1, #goblin_weapons)])

      om:wear(goblin, shield)
      om:wear(goblin, helmet)
      om:equip(goblin, Paperdoll.MAIN_HAND, mace)
      --om:add_combat_ability(goblin, 'module://stonehearth/combat_abilities/shield_block.txt')
      om:add_combat_ability(goblin, 'module://stonehearth/combat_abilities/shield_swing.txt')
      
      om:set_attribute(goblin, 'health', 1)
      
      om:place_on_terrain(goblin, RadiantIPoint3(x, 1, z))
      om:get_component(goblin, 'mob'):turn_to(math.random(0, 359))     
      
      ai_mgr:add_intention(goblin, 'radiant.intentions.combat_defense')
      local abilities = {
         'module://stonehearth/combat_abilities/shield_block.txt',
         'module://stonehearth/combat_abilities/shield_swing.txt',
      }
      for _, ability_name in ipairs(abilities) do
         om:add_combat_ability(goblin, ability_name)
      end
      table.insert(goblins, goblin)
      return goblin
   end
   local spawn_soldier = function(x, z)
      local footman = sh:create_citizen(RadiantIPoint3(x, 1, z), 'footman')
      om:add_combat_ability(footman, 'module://stonehearth/combat_abilities/parry.txt')
      ai_mgr:add_intention(footman, 'radiant.intentions.combat_defense')
      local sword = om:create_entity('module://stonehearth/items/iron_sword')
      om:equip(footman, Paperdoll.MAIN_HAND, sword)
      table.insert(soldiers, footman)
      return footman
   end
   
   -- on the road leading to the tower...
   --[[
   spawn_goblin(283, 247, true)
   spawn_goblin(287, 264, true)
   ]]
   -- in the woods..
   spawn_goblin(296, 157)
   spawn_goblin(292, 173)
   spawn_goblin(288, 186)
   spawn_goblin(311, 194)
   spawn_goblin(309, 302)
   spawn_goblin(281, 162)
   spawn_goblin(309, 147)
   spawn_goblin(331, 193)
   spawn_goblin(309, 204)
   spawn_goblin(296, 174)
   spawn_goblin(311, 222)
   
   spawn_soldier(245, 211)
   spawn_soldier(254, 218)
   spawn_soldier(260, 224)
   spawn_soldier(264, 231)
   spawn_soldier(268, 238)
   spawn_soldier(272, 245)
   
   -- stable marriage... of a sorts.
   local candidates = {}
   local goblin_targets = {}
   
   while #goblins > 0 do
      local best_i, best_j, best_d
      for i, goblin in ipairs(goblins) do
         for j, soldier in ipairs(soldiers) do
            local d = om:get_distance_between(soldier, goblin)
            if not best_d or d < best_d then
               best_d = d
               best_j = j
               best_i = i
            end         
         end
      end
      table.insert(goblin_targets, {
         goblin = goblins[best_i],
         soldier = soldiers[best_j]
      })
      table.remove(goblins, best_i)
      --table.remove(candidates, best_j)
   end
   
   local done
   md:listen('radiant.events.gameloop', function (_, now)    
      if not done and now > 10000 then
         for i, entry in ipairs(goblin_targets) do
            log:warning('targeting goblin %d to solder %d', entry.goblin:get_id(), entry.soldier:get_id())
            local tt = om:create_target_table(entry.goblin, 'radiant.aggro')
            tt:add_entry(entry.soldier):set_value(1)
         end
         done = true
      end
   end)
end

function scene_6()
   scene_4()
   local goblins = {}
   local soldiers = {}
   local goblin_weapons = {
      'module://stonehearth/items/spiked_wooden_mace'
   }
   local spawn_goblin = function(x, z)
      local goblin = om:create_entity('module://stonehearth/mobs/goblin_soldier')
      local shield = om:create_entity('module://stonehearth/items/cracked_wooden_buckler')
      local helmet = om:create_entity('module://stonehearth/items/studded_leather_helmet')
      local mace = om:create_entity(goblin_weapons[math.random(1, #goblin_weapons)])

      om:wear(goblin, shield)
      om:wear(goblin, helmet)
      om:equip(goblin, Paperdoll.MAIN_HAND, mace)
      --om:add_combat_ability(goblin, 'module://stonehearth/combat_abilities/shield_block.txt')
      om:add_combat_ability(goblin, 'module://stonehearth/combat_abilities/shield_swing.txt')
      
      om:set_attribute(goblin, 'health', math.random(16, 29))
      
      om:place_on_terrain(goblin, RadiantIPoint3(x, 1, z))
      om:get_component(goblin, 'mob'):turn_to(math.random(0, 359))     
      
      ai_mgr:add_intention(goblin, 'radiant.intentions.combat_defense')
      local abilities = {
         'module://stonehearth/combat_abilities/shield_block.txt',
         'module://stonehearth/combat_abilities/shield_swing.txt',
      }
      for _, ability_name in ipairs(abilities) do
         om:add_combat_ability(goblin, ability_name)
      end
      table.insert(goblins, goblin)
      return goblin
   end
   local spawn_soldier = function(x, z)
      local weapons = {
         'radiant.items.short_sword',
      }
      local footman = sh:create_citizen(RadiantIPoint3(x, 1, z), 'footman')
      om:add_combat_ability(footman, 'module://stonehearth/combat_abilities/parry.txt')
      ai_mgr:add_intention(footman, 'radiant.intentions.combat_defense')
      local sword = om:create_entity(weapons[math.random(#weapons)])
      om:equip(footman, Paperdoll.MAIN_HAND, sword)
      table.insert(soldiers, footman)
      return footman
   end
   
   local width = 5
   local x, y = 246, 200
   local rd = function(x) return x + math.random(-3, 3) end

   local goblin_targets = {}
   local soldiers = {}
   for i = 1, width do
      table.insert(soldiers, spawn_soldier(rd(x) + i * 4, rd(y) + i * 7))
   end
   
   local spawn_wave = function(d)
      for i, soldier in ipairs(soldiers) do
         table.insert(goblin_targets, {
            goblin = spawn_goblin(rd(x) + d + i * 4, rd(y) - d + i * 7),
            soldier = soldier
         })
      end
   end
   spawn_wave(30)
   spawn_wave(90)
   spawn_wave(150)
   
   
   local done
   md:listen('radiant.events.gameloop', function (_, now)    
      if not done and now > 10000 then
         for i, entry in ipairs(goblin_targets) do
            log:warning('targeting goblin %d to solder %d', entry.goblin:get_id(), entry.soldier:get_id())
            local tt = om:create_target_table(entry.goblin, 'radiant.aggro')
            tt:add_entry(entry.soldier):set_value(100000)
         end
         done = true
      elseif not done then
         log:warning('.... %d ...', now)
      end
   end)
end

function scene_7()
   scene_4()
   local boss = om:create_entity('radiant.mobs.cthulu')
   om:place_on_terrain(boss, RadiantIPoint3(329, 1, 195))
   
   local done = false
   md:listen('radiant.events.gameloop', function (_, now)    
      if not done and now > 4000 then
         ai_mgr:add_intention(boss, 'radiant.intentions.cthulu')
         done = true
      end
   end)
   --om:get_component(goblin, 'mob'):turn_to(math.random(0, 359))
end

function scene_8()
   local add = function(name, x, z, angle)
      local entity = om:create_entity('radiant.mobs.' .. name)
      om:place_on_terrain(entity, RadiantIPoint3(x, 1, z))
      angle = angle and angle or math.random(0, 359)
      om:get_component(entity, 'mob'):turn_to(angle) 
   end
   
   add('mammoth', 446, 180)
   add('sheep', 454, 181)
   add('sheep', 451, 188)
   add('sheep', 447, 191)
   add('sheep', 442, 193)
   add('sheep', 437, 194)
   add('sheep', 439, 189)
   add('sheep', 434, 189)
   add('shepard', 439, 184, 180)
end

function GameMaster:start_new_game()
   local size = 32

   tg:create()
   scene_7();
   --[[
   -- Put the embark smack in the middle
   self:start_scenario('stonehearth.embark', RadiantBounds3(RadiantIPoint3(240, 1, 240), RadiantIPoint3(272, 1, 272)))
   
   -- Put a goblin camp opposite the forsest
   local bounds = RadiantBounds3(RadiantIPoint3(50,   1, 50),
                                 RadiantIPoint3(100,  1, 100))
   table.insert(self._scenarios, self:start_scenario('radiant.scenarios.goblin_camp', bounds))
   
   for _, scenario in ipairs(self._scenarios) do
      md:send_msg(scenario, 'radiant.scenario.set_run_level', Scenario.RunLevel.ACTIVE)
   end
   ]]
end

function GameMaster:run_test(name)
   local bounds = RadiantBounds3(RadiantIPoint3(-16, 1, -16), RadiantIPoint3(16, 1, 16))
   table.insert(self._scenarios, self:start_scenario(name, bounds))
   for _, scenario in ipairs(self._scenarios) do
      md:send_msg(scenario, 'radiant.scenario.set_run_level', Scenario.RunLevel.ACTIVE)
   end
end

return GameMaster()