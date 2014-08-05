local Entity = _radiant.om.Entity

local HarvestTrappedBeast = class()

HarvestTrappedBeast.name = 'harvest trapped beast'
HarvestTrappedBeast.does = 'stonehearth:trapping:check_bait_trap_adjacent'
HarvestTrappedBeast.args = {
   trap = Entity
}
HarvestTrappedBeast.version = 2
HarvestTrappedBeast.priority = 1
HarvestTrappedBeast.weight = 20 -- TODO: determine probability of tame vs harvest dynamically

function HarvestTrappedBeast:run(ai, entity, args)
   self._entity = entity

   if not args.trap:is_valid() then
      self:stop()
      ai:abort('trap is destroyed')
      return
   end

   local trap_component = args.trap:add_component('stonehearth:bait_trap')
   local trapped_entity = trap_component:get_trapped_entity()

   ai:execute('stonehearth:turn_to_face_entity', { entity = args.trap })
   ai:execute('stonehearth:run_effect', { effect = 'fiddle' })

   if trapped_entity and trapped_entity:is_valid() then
      self:_gib_target(trapped_entity)
      self:_spawn_loot(trapped_entity)
      radiant.entities.destroy_entity(trapped_entity)
      trapped_entity = nil
   end

   radiant.entities.destroy_entity(args.trap)
end

function HarvestTrappedBeast:_gib_target(target)
   local location = radiant.entities.get_world_grid_location(target)

   local proxy_entity = radiant.entities.create_entity()
   radiant.terrain.place_entity(proxy_entity, location)

   local effect = radiant.effects.run_effect(proxy_entity, '/stonehearth/data/effects/gib_effect')

   effect:set_finished_cb(
      function ()
         radiant.entities.destroy_entity(proxy_entity)
      end
   )
end

function HarvestTrappedBeast:_spawn_loot(target)
   local loot_table_component = target:get_component('stonehearth:loot_table')
   if not loot_table_component then
      return
   end

   local item_uris = loot_table_component:get_loot()
   if not item_uris then
      return
   end

   local origin = radiant.entities.get_world_grid_location(target)

   for _, item_uri in pairs(item_uris) do
      local item = radiant.entities.create_entity(item_uri)

      -- lease the item so nobody else picks it up
      -- leases are not ref counted, so this lease will be released when the restock action completes (or aborts)
      -- if the entity never picks up the item and never dies, how does this lease get released?
      local leased = stonehearth.ai:acquire_ai_lease(item, self._entity)
      -- we just created it, we better be able to lease it!
      assert(leased)

      -- place the item after acquiring the lease
      local location = radiant.terrain.find_placement_point(origin, 1, 2)
      radiant.terrain.place_entity(item, location)
      
      self:_create_loot_item_task(item)
   end
end

function HarvestTrappedBeast:_create_loot_item_task(item)
   local task = self._entity:add_component('stonehearth:ai')
      :get_task_group('stonehearth:trapping')
      :create_task('stonehearth:loot_item', { item = item })
      :set_priority(stonehearth.constants.priorities.trapping.PICK_UP_LOOT)
      :once()
      :start()
end

return HarvestTrappedBeast
