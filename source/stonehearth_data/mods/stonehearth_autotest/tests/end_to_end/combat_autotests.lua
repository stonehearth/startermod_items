local Point3 = _radiant.csg.Point3

local combat_tests = {}

function combat_tests.battle_royale(autotest)
   local footmen = {
      autotest.env:create_person(-15, -15, { profession = 'footman', talisman = 'stonehearth:wooden_sword' }),
      autotest.env:create_person( -7, -15, { profession = 'footman', talisman = 'stonehearth:wooden_sword' }),
      autotest.env:create_person(  1, -15, { profession = 'footman', talisman = 'stonehearth:wooden_sword' }),
      autotest.env:create_person(  9, -15, { profession = 'footman', talisman = 'stonehearth:wooden_sword' }),
   }

   local enemies = {
      autotest.env:create_person( -9, 15, { player_id = 'enemy', weapon = 'stonehearth:wooden_sword' }),
      autotest.env:create_person( -1, 15, { player_id = 'enemy', weapon = 'stonehearth:wooden_sword' }),
      autotest.env:create_person(  7, 15, { player_id = 'enemy', weapon = 'stonehearth:wooden_sword' }),
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
   local stockpile1 = autotest.env:create_stockpile(30, 30, { size = { x = 2, y = 2 }})
   local stockpile2 = autotest.env:create_stockpile(20, 20, { size = { x = 2, y = 2 }})
   local footman = autotest.env:create_person(28, 28, { profession = 'footman', talisman = 'stonehearth:wooden_sword' })
   local patrol_point = radiant.entities.get_world_grid_location(stockpile2) - Point3(2, 0, 2)

   local trace
   trace = radiant.entities.trace_location(footman, 'patrol autotest')
      :on_changed(
         function()
            local location = radiant.entities.get_world_grid_location(footman)
            if location == patrol_point then
               autotest:success()
               trace:destroy()
            end
         end
      )

   autotest:sleep(12000)
   trace:destroy()
   autotest:fail('patrol failed to complete on time')
end

function combat_tests.worker_defense(autotest)
   local workers = {
      autotest.env:create_person(-15, -15, { profession = 'worker' }),
      autotest.env:create_person( -7, -15, { profession = 'carpenter', talisman = 'stonehearth:carpenter:saw' }),
      autotest.env:create_person(  1, -15, { profession = 'farmer', talisman = 'stonehearth:farmer:hoe' }),
      autotest.env:create_person(  9, -15, { profession = 'trapper', talisman = 'stonehearth:trapper:knife' }),
   }

   local enemies = {
      autotest.env:create_person( -9, 15, { player_id = 'enemy', weapon = 'stonehearth:wooden_sword' }),
      autotest.env:create_person( -1, 15, { player_id = 'enemy', weapon = 'stonehearth:wooden_sword' }),
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

return combat_tests
