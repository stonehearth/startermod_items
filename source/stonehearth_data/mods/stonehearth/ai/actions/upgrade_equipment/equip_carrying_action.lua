local entity = _radiant.om.Entity

local EquipCarrying = class()
EquipCarrying.name = 'equip carrying'
EquipCarrying.does = 'stonehearth:equip_carrying'
EquipCarrying.args = {}
EquipCarrying.version = 2
EquipCarrying.priority = 1

function EquipCarrying:start_thinking(ai, entity, args)
   if ai.CURRENT.carrying then
      local item = ai.CURRENT.carrying
      local _, equipment_piece = radiant.entities.unwrap_iconic_item(item, 'stonehearth:equipment_piece')
      if equipment_piece then
         ai.CURRENT.carrying = nil
         ai:set_think_output({})
      end
   end
end

function EquipCarrying:run(ai, entity, args)
   local item = radiant.entities.get_carrying(entity)
   local _, equipment_piece = radiant.entities.unwrap_iconic_item(item, 'stonehearth:equipment_piece')

   if not equipment_piece then
      ai:abort('not carrying an equipable item!')
   end
   radiant.entities.remove_carrying(entity)

   local old_item = entity:add_component('stonehearth:equipment')
                              :equip_item(item)
   if old_item then
      local ilevel = old_item:get_component('stonehearth:equipment_piece')
                                 :get_ilevel()
      if ilevel == 0 then
         radiant.entities.destroy_entity(old_item)
      else
         local location = radiant.entities.get_world_grid_location(entity)
         local drop_location = radiant.terrain.find_placement_point(location, 0, 2)
         radiant.terrain.place_entity(old_item, drop_location)
      end
   end
end

return EquipCarrying
