local Point3 = _radiant.csg.Point3

local singleton = {
   _schedulers = {}
}
local worker_class = {}

local worker_weapons = { 'stonehearth:mining_pick', 'stonehearth:worker_hammer' }
local rng = _radiant.csg.get_default_rng()

function worker_class.promote(entity)
   worker_class.restore(entity)

   -- TODO: make workers fight with the last tool they were using
   local weapon = radiant.entities.create_entity(worker_weapons[rng:get_int(1, 2)])
   radiant.entities.equip_item(entity, weapon, 'melee_weapon')

   stonehearth.combat:set_stance(entity, 'passive')
end

function worker_class.restore(entity)
   --local town = stonehearth.town:get_town(entity)
   --town:join_task_group(entity, 'workers')
end

function worker_class.demote(entity)
   --local town = stonehearth.town:get_town(entity)
   --town:leave_task_group(entity, 'workers')
end

return worker_class
