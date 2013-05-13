require 'unclasslib'
local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local gm = require 'radiant.core.gm'
local log = require 'radiant.core.log'
local ai_mgr = require 'radiant.ai.ai_mgr'

local GoblinCamp = class()

GoblinCamp['radiant.md.create'] = function(self, options)
   self._options = options
   self._raid_stockpile_candidates = {}
   self._goblin_warriors = {}
   self._goblin_workers = {}
end

function GoblinCamp:_create_goblin(center, kind)
   local location = RadiantIPoint3()
   location.x = center.x + math.random(-8, 8)
   location.y = center.y
   location.z = center.z + math.random(-8, 8)

   local goblin = om:create_entity(kind)
   om:place_on_terrain(goblin, location)
   om:get_component(goblin, 'mob'):turn_to(math.random(0, 359))

   return goblin
end

function GoblinCamp:_create_goblins()
   local center = self._options.box:center()
 
   -- create a stockpile in the center of the camp...
   local stockpile = om:create_entity('stockpile')
   om:place_on_terrain(stockpile, RadiantIPoint3(center))
   om:add_component(stockpile, 'unit_info'):set_faction('goblin')
   om:add_component(stockpile, 'stockpile_designation'):set_bounds(RadiantBounds3(RadiantIPoint3(-4, 0, -4),
                                                                                  RadiantIPoint3( 4, 1,  4)))
   local designation = om:get_component(stockpile, 'stockpile_designation')
   designation:set_container(om:get_component(om:get_root_entity(), 'entity_container')) -- xxx do this automatically as part of becoming a child?                                                                                  
   self._home_stockpile = stockpile

   -- create the scheduler
   self._worker_scheduler = md:create_msg_handler('radiant.msg_handlers.worker_scheduler')
   self._worker_scheduler:add_stockpile(self._home_stockpile)
   
   -- create the workers
   log:info('inside goblin camp... (%s)', tostring(self._options.box))
   self._goblins = {}
   for i = 1, 4 do
      local goblin = self:_create_goblin(center, 'radiant.mobs.goblin-worker')
      om:add_component(goblin, 'carry_block');
      self._intention = ai_mgr:add_intention(goblin, 'radiant.intentions.worker_scheduler_slave', self._worker_scheduler)
      
      self._goblin_workers[goblin:get_id()] = goblin
   end

   -- create the soldiers
   for i = 1, 2 do
      local goblin = self:_create_goblin(center, 'module://stonehearth/mobs/goblin_soldier')
      local club = om:create_entity('radiant.weapons.1h.club')
      om:equip(goblin, Paperdoll.MAIN_HAND, club)
      self._goblin_warriors[goblin:get_id()] = goblin

      ai_mgr:add_intention(goblin, 'radiant.intentions.protect_stockpile', stockpile, 4, 5)
   end
   
end

function GoblinCamp:_watch_for_stockpiles()
   local container = om:get_component(om:get_root_entity(), 'entity_container')

   local added = function(id, entity)
      if om:has_component(entity, 'stockpile_designation') and om:get_faction(entity) ~= 'goblin' then
         self._raid_stockpile_candidates[id] = entity
      end
   end
   local removed = function(id, entity)
      self._raid_stockpile_candidates[id] = nil
   end
   
   self._stockpile_promise = container:trace_children()
                              :added(added)
                              :removed(removed)
end

GoblinCamp['radiant.scenario.set_run_level'] = function(self, level)
   if self._runlevel ~= level then
      if level == Scenario.RunLevel.ACTIVE then
         self:_create_goblins()
         self:_watch_for_stockpiles();
         md:listen('radiant.events.very_slow_poll', self)
      end   
   end
end

GoblinCamp['radiant.events.very_slow_poll'] = function(self, level)
   if not self._raiding_stockpile then
      self:_choose_raiding_stockpile()
      return
   else
      self:_check_raid_progress()
   end

end

function GoblinCamp:_choose_raiding_stockpile()
   assert(not self._raiding_stockpile)
   
   local total_items = 0
   local closest_stockpile;
   local closest_distance;
   local origin = RadiantIPoint3(self._options.box:center())
   
   for id, entity in pairs(self._raid_stockpile_candidates) do
      local stockpile = om:get_component(entity, 'stockpile_designation')
      local item_count = stockpile:get_contents():size()
      total_items = total_items + item_count
      if item_count > 0 then
         local position = om:get_world_grid_location(entity)
         local d = (origin - position):distance_squared()
         if not closest_distance or d < closest_distance then
            closest_distance = d
            closest_stockpile = stockpile
         end
      end
   end
   if total_items > 4 and closest_stockpile then
      self:_start_raiding(closest_stockpile)
   end
end

function GoblinCamp:_start_raiding(stockpile)
   assert(not self._raiding_stockpile)
   
   self._raiding_intentions = {}

   -- Create a worker scheduler to coordinate the workers
   self._worker_scheduler:start()
   
   -- have the warriors stake out random locations in the stockpile
   local stake_out = om:get_world_grid_location(stockpile:get_entity())
   for id, soldier in pairs(self._goblin_warriors) do
      local intention = ai_mgr:add_intention(soldier, 'radiant.intentions.protect_stockpile', stockpile:get_entity(), 4, 10)
      table.insert(self._raiding_intentions, intention)
   end

   -- track the items in the stcokpile
   self._raiding_stockpile = stockpile
   self._raiding_stockpile_promise = stockpile:get_contents():trace()
                                       :added(function(id)
                                             self:_add_stocked_item_to_scheduler(id)                                          
                                          end)
                                       
   for id in stockpile:get_contents():items() do
      self:_add_stocked_item_to_scheduler(id);
   end   
   -- order the soliders to protect the workers
end

function GoblinCamp:_check_raid_progress()
   assert (self._raiding_stockpile)
   if self._raiding_stockpile:get_contents():is_empty() then
      self:_stop_raiding()
   end
end

function GoblinCamp:_stop_raiding(stockpile)
   for _, intention in ipairs(self._raiding_intentions) do
      intention:destroy()
   end

   self._worker_scheduler:stop()
   self._raiding_intentions = {}
   self._raiding_stockpile = nil
end

function GoblinCamp:_add_stocked_item_to_scheduler(id)
   local entity = om:get_entity(id)
   if entity and om:has_component(entity, 'item') then
      self._worker_scheduler:add_item(entity)
   end
end

gm:register_scenario('radiant.scenarios.goblin_camp', GoblinCamp)

