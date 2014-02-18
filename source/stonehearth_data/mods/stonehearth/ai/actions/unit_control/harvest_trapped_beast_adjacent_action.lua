local HarvestTrappedBeastAdjacent = class()

local Entity = _radiant.om.Entity

HarvestTrappedBeastAdjacent.name = 'harvest trapped beast adjacent'
HarvestTrappedBeastAdjacent.does = 'stonehearth:harvest_trapped_beast_adjacent'
HarvestTrappedBeastAdjacent.args = {
   trap = Entity
}
HarvestTrappedBeastAdjacent.version = 2
HarvestTrappedBeastAdjacent.priority = stonehearth.constants.priorities.unit_control.CAST_SPELL

function HarvestTrappedBeastAdjacent:run(ai, entity, args)
   local trap = args.trap
   
   if not trap or not trap:is_valid() then
      return
   end
   
   local _, beast = trap:get_component('entity_container'):first_child()

   if beast and beast:is_valid() then 
      radiant.entities.remove_buff(beast, 'stonehearth:buffs:snared')

      -- kill the beast and spawn loot from it
      --radiant.effects.run_effect(beast, '/stonehearth/data/effects/firepit_effect')
      -- xxx, put this effect on a proxy-entity instead
      radiant.effects.run_effect(beast, '/stonehearth/data/effects/gib')
      self:_spawn_loot(beast)
      radiant.entities.destroy_entity(beast)
   end
end

function HarvestTrappedBeastAdjacent:_spawn_loot(beast)
   local loot_table_component = beast:get_component('stonehearth:loot_table')
   if loot_table_component then
      loot_table_component:spawn_loot()
   end
end


return HarvestTrappedBeastAdjacent
