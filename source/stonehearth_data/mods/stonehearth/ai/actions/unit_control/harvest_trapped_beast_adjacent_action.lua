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
   end

end


return HarvestTrappedBeastAdjacent
