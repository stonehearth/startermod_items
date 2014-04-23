local HarvestTrappedBeastAdjacent = class()

local Entity = _radiant.om.Entity

HarvestTrappedBeastAdjacent.name = 'harvest trapped beast adjacent'
HarvestTrappedBeastAdjacent.does = 'stonehearth:unit_control:harvest_trapped_beast_adjacent'
HarvestTrappedBeastAdjacent.args = {
   trap = Entity
}
HarvestTrappedBeastAdjacent.version = 2
HarvestTrappedBeastAdjacent.priority = stonehearth.constants.priorities.unit_control.CAST_SPELL

function HarvestTrappedBeastAdjacent:start(ai, entity, args)
   self._entity = entity
   self._ai = ai
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

function HarvestTrappedBeastAdjacent:stop(ai, entity, args)
   self._entity = nil
   self._ai = nil
end

function HarvestTrappedBeastAdjacent:_harvest_beast(beast)   
   local loot = nil 
   if beast and beast:is_valid() then 
      radiant.entities.remove_buff(beast, 'stonehearth:buffs:snared')

      -- kill the beast and spawn loot from it
      self:_gib_beast(beast)
      loot = self:_spawn_loot(beast)
      radiant.entities.destroy_entity(beast)
   end

   return loot
end

function HarvestTrappedBeastAdjacent:_gib_beast(beast)
   local location = radiant.entities.get_world_grid_location(beast)

   local proxy_entity = radiant.entities.create_entity()
   radiant.terrain.place_entity(proxy_entity, location)

   local effect = radiant.effects.run_effect(proxy_entity, '/stonehearth/data/effects/gib_effect')

   -- This assumes the effect always finishes and nothing else can stop it
   effect:set_finished_cb(
      function ()
         radiant.entities.destroy_entity(proxy_entity)
      end
   )
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

      -- lease the item.  
      -- xxx - we need a way to break this lease whenever the scheduled command gets removed.
      -- otherwise, workers can't come clean up after the trapper's mess if he gets interrupted
      -- before he can carry out those commands.
      if not stonehearth.ai:acquire_ai_lease(item, self._entity) then
         radiant.entities.destroy_entity(item)
         self:stop() -- in case stop is not called on abort
         self._ai:abort('could not lease %s.', tostring(item))
         return
      end

      radiant.terrain.place_entity(item, location)
      
      self:_schedule_pickup_item(item)
   end

   return nil
end

function HarvestTrappedBeastAdjacent:_schedule_pickup_item(item)
   local town = stonehearth.town:get_town(self._entity)
   if town then
      town:command_unit_scheduled(self._entity, 'stonehearth:loot_item', { item = item })
            :once()
            :start() 
   end
end

return HarvestTrappedBeastAdjacent
