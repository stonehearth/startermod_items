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

   for _, entity in pairs(enemies) do
      -- make goblins auto attack
      stonehearth.ai:inject_ai(entity, { observers = { "stonehearth:observers:enemy_observer" }})
   end

   local function any_valid_entities(entities)
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

return combat_tests
