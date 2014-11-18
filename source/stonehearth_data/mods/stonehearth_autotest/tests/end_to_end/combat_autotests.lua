local Point3 = _radiant.csg.Point3

local combat_tests = {}

function combat_tests.battle_royale(autotest)
   local footmen = {
      autotest.env:create_person(-15, -15, { job = 'footman' }),
      autotest.env:create_person( -7, -15, { job = 'footman' }),
      autotest.env:create_person(  1, -15, { job = 'footman' }),
      autotest.env:create_person(  9, -15, { job = 'footman' }),
   }

   local enemies = {
      autotest.env:create_person( -9, 15, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword' }),
      autotest.env:create_person( -1, 15, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword' }),
      autotest.env:create_person(  7, 15, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword' }),
   }

   local any_valid_entities = function(entities)
      for _, entity in pairs(entities) do
         if entity:is_valid() then
            return true
         end
      end
      return false
   end

   radiant.events.listen(radiant, 'radiant:entity:post_destroy',
      function()
         if not any_valid_entities(enemies) then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end
   )

   autotest:sleep(20000)
   autotest:fail('combat failed to complete on time')
end

function combat_tests.patrol(autotest)
   local stockpile1 = autotest.env:create_stockpile(-5, -5, { size = { x = 2, y = 2 }})
   local stockpile2 = autotest.env:create_stockpile(0, 0, { size = { x = 2, y = 2 }})
   local footman = autotest.env:create_person(5, 5, { job = 'footman' })
   local patrol_point = radiant.entities.get_world_grid_location(stockpile1) - Point3(2, 0, 2)

   local trace
   trace = radiant.entities.trace_grid_location(footman, 'patrol autotest')
      :on_changed(
         function()
            local location = radiant.entities.get_world_grid_location(footman)
            if location == patrol_point then
               autotest:success()
               trace:destroy()
            end
         end
      )

   autotest:sleep(45000)
   trace:destroy()
   autotest:fail('patrol failed to complete on time')
end

function combat_tests.worker_defense(autotest)
   local workers = {
      autotest.env:create_person(-15, -15, { job = 'worker' }),
      autotest.env:create_person( -7, -15, { job = 'carpenter' }),
      autotest.env:create_person(  1, -15, { job = 'farmer' }),
      autotest.env:create_person(  9, -15, { job = 'trapper' }),
   }

   local enemies = {
      autotest.env:create_person( -9, 15, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword' }),
      autotest.env:create_person( -1, 15, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword' }),
   }

   local any_valid_entities = function(entities)
      for _, entity in pairs(entities) do
         if entity:is_valid() then
            return true
         end
      end
      return false
   end

   radiant.events.listen(radiant, 'radiant:entity:post_destroy',
      function()
         if not any_valid_entities(enemies) then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end
   )

   local town = stonehearth.town:get_town(autotest.env.get_player_id())
   town:enable_worker_combat()

   autotest:sleep(20000)
   autotest:fail('woker defense failed to complete on time')
end



function combat_tests.talsman_drop(autotest)
   local citizens = {
      autotest.env:create_person(-9, -2, { job = 'footman', attributes = { mind = 0, body = 1, spirit = 0 } }),
      autotest.env:create_person( -5, -2, { job = 'carpenter', attributes = { mind = 0, body = 1, spirit = 0 } }),
      autotest.env:create_person(  1, -2, { job = 'farmer', attributes = { mind = 0, body = 1, spirit = 0 } }),
      autotest.env:create_person(  3, -2, { job = 'trapper', attributes = { mind = 0, body = 1, spirit = 0 } }),
      autotest.env:create_person(  5, -2, { job = 'weaver', attributes = { mind = 0, body = 1, spirit = 0 } })
   }


   --Note: sometimes tombstones pop in under a person, preventing them from being attacked
   --I've fixed the tombstones so they should check the terrain before they pop.
   --If that doesnt work you can comment out the goblins and comment in this:  
   --Temp fix: Make everyone's HP go down manually 
   --[[
   autotest:sleep(200)
   for i, citizen in ipairs(citizens) do 
      local attrib_component = citizen:add_component('stonehearth:attributes')
      attrib_component:set_attribute('health', -1)
   end
   ]]

   ---[[
   autotest.env:create_person( -9, 0, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword', attributes = { body = 100} })
   autotest.env:create_person( -5, 0, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword', attributes = { body = 100}  })
   autotest.env:create_person( 1, 0, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword', attributes = { body = 100} })
   autotest.env:create_person( 3, 0, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword', attributes = { body = 100}  })
   autotest.env:create_person( 5, 0, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword', attributes = { body = 100}  })
   autotest.env:create_person( -12, 0, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword', attributes = { body = 100}  })
   autotest.env:create_person( 6, 0, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword', attributes = { body = 100} })
   autotest.env:create_person( 0, 0, { player_id = 'enemy', weapon = 'stonehearth:weapons:wooden_sword', attributes = { body = 100} })
   --]]


   local num_talismans = 0

   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:carpenter:saw_talisman' or 
         e.entity:get_uri() == 'stonehearth:weaver:talisman' or 
         e.entity:get_uri() == 'stonehearth:farmer:talisman' or
         e.entity:get_uri() == 'stonehearth:trapper:knife_talisman' or
         e.entity:get_uri() == 'stonehearth:footman:wooden_sword_talisman' then
         num_talismans = num_talismans + 1
      end
      if num_talismans == 5 then
         autotest:success()
         return radiant.events.UNLISTEN
      end
   end)

   autotest:sleep(200000)
   autotest:fail('talisman_drop failed to complete on time')
end


return combat_tests
