local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local UnloadInventory = class()

UnloadInventory.name = 'unload inventory'
UnloadInventory.does = 'stonehearth:unit_control:unload_inventory'
UnloadInventory.args = {
   
}

UnloadInventory.version = 2
UnloadInventory.priority = stonehearth.constants.priorities.unit_control.CAST_SPELL

local ai = stonehearth.ai
return ai:create_compound_action(UnloadInventory)
         :execute('stonehearth:goto_town_banner')
         :execute('stonehearth:drop_inventory_items')
