local HarvestTrappedBeastAdjacent = class()

local Entity = _radiant.om.Entity

HarvestTrappedBeastAdjacent.name = 'harvest trapped beast adjacent'
HarvestTrappedBeastAdjacent.does = 'stonehearth:harvest_trapped_beast_adjacent'
HarvestTrappedBeastAdjacent.args = {
   trap = Entity
}
HarvestTrappedBeastAdjacent.version = 2
HarvestTrappedBeastAdjacent.priority = stonehearth.constants.priorities.unit_control.CAST_SPELL

function HarvestTrappedBeastAdjacent:start(ai, entity, args)
   self._entity = entity
end

function HarvestTrappedBeastAdjacent:run(ai, entity, args)
   local trap = args.trap
   
   if not trap or not trap:is_valid() then
      return
   end
   
   local _, beast = trap:get_component('entity_container'):first_child()

   -- spawn the loot and get rid of the beast
   local loot = self:_harvest_beast(beast)

   -- go pickup the loot
   if loot then 
      self:_pickup_loot(loot)
   end
end

function HarvestTrappedBeastAdjacent:_harvest_beast(beast)   
   local loot = nil 
   if beast and beast:is_valid() then 
      radiant.entities.remove_buff(beast, 'stonehearth:buffs:snared')

      -- kill the beast and spawn loot from it
      --radiant.effects.run_effect(beast, '/stonehearth/data/effects/firepit_effect')
      -- xxx, put this effect on a proxy-entity instead
      radiant.effects.run_effect(beast, '/stonehearth/data/effects/gib')
      loot = self:_spawn_loot(beast)
      radiant.entities.destroy_entity(beast)
   end

   return loot
end

function HarvestTrappedBeastAdjacent:_spawn_loot(beast)
   local loot_table_component = beast:get_component('stonehearth:loot_table')

   if not loot_table_component then
      return
   end

   local item_uris = loot_table_component:get_loot()

   if not item_uris then
      return
   end

   local location = radiant.entities.get_world_grid_location(self._entity)
   for _, item_uri in ipairs(item_uris) do
      local item = radiant.entities.create_entity(item_uri)
      location.x = location.x + math.random(-1, 1)
      location.z = location.z + math.random(-1, 1)

      -- lease the item
      local lease_component = item:add_component('stonehearth:lease')
      if not lease_component:acquire(stonehearth.ai.RESERVATION_LEASE_NAME, self._entity) then
         ai:abort('could not lease %s (%s has it).', tostring(item), tostring(lease_component:get_owner(stonehearth.ai.RESERVATION_LEASE_NAME)))
         return
      end

      radiant.terrain.place_entity(item, location)
      
      self:_schedule_pickup_item(item)
   end

   return nil
end

function HarvestTrappedBeastAdjacent:_schedule_pickup_item(item)
   local faction = radiant.entities.get_faction(self._entity)
   local town = stonehearth.town:get_town(faction)

   town:command_unit_scheduled(self._entity, 'stonehearth:loot_item', { item = item })
         --:add_entity_effect(loot_item, '/stonehearth/data/effects/chop_overlay_effect')
         :once()
         :start()

end

return HarvestTrappedBeastAdjacent
