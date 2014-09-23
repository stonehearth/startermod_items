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

   local trap_component = args.trap:add_component('stonehearth:bait_trap')
   local trapped_entity = trap_component:get_trapped_entity()
   local trapped_entity_id = nil

   ai:execute('stonehearth:turn_to_face_entity', { entity = args.trap })
   ai:execute('stonehearth:run_effect', { effect = 'fiddle' })

   if trapped_entity and trapped_entity:is_valid() then
      trapped_entity_id = trapped_entity:get_id()
      self:_gib_target(trapped_entity)
      self:_spawn_loot(trapped_entity)
      radiant.entities.destroy_entity(trapped_entity)
      trapped_entity = nil
   end

   ai:unprotect_entity(args.trap)
   radiant.entities.destroy_entity(args.trap)

   radiant.events.trigger_async(entity, 'stonehearth:clear_trap', {trapped_entity_id = trapped_entity_id})
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

   local items = loot_table_component:spawn_loot()

   for id, item in pairs(items) do
      -- lease the item so nobody else picks it up
      -- leases are not ref counted, so this lease will be released after we pick up the item
      -- if the entity never picks up the item and never dies, how does this lease get released?
      local leased = stonehearth.ai:acquire_ai_lease(item, self._entity)

      if leased then
         self:_create_loot_item_task(item)
      end
   end
end

function HarvestTrappedBeast:_create_loot_item_task(item)
   local task = self._entity:add_component('stonehearth:ai')
      :get_task_group('stonehearth:trapping')
      :create_task('stonehearth:pickup_item_into_backpack', { item = item })
      :set_priority(stonehearth.constants.priorities.trapping.PICK_UP_LOOT)
      :once()
      :start()
end

return HarvestTrappedBeast
