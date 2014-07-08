local constants = require 'constants'
local log = radiant.log.create_logger('combat')

-- Key nodes in the combat AI tree
-- Format is:
--
-- Action(Priority) - Activity
-- 
-- combat(100) - stonehearth:top
--    attack_distpatcher(10) - stonehearth:combat
--       attack_melee(1) - stonehearth:combat:attack
--          attack_melee_adjacent(1) - stonehearth:combat:attack_melee_adjacent
--       attack_ranged(2) - stonehearth:combat:attack
--       attack_spell(3) - stonehearth:combat:attack
--    defend_dispatcher(10) - stonehearth:combat
--       defend_melee(1) - stonehearth:combat:defend
--       defend_ranged(1) - stonehearth:combat:defend
--    combat_idle_dispatcher(1) - stonehearth:combat
--       combat_idle_shuffle(2) - stonehearth:combat:idle
--       combat_idle_ready(1) - stonehearth:combat:idle
--    panic_dispatcher(20) - stonehearth:combat
--       flee(1) - stonehearth:combat:panic
-- compelled_behavior_dispatcher(9999999) - stonehearth:top
--    hit_stun(100) - stonehearth:compelled_behavior

local Combat = class()
Combat.name = 'combat'
Combat.status_text = 'engaged in combat'
Combat.does = 'stonehearth:top'
Combat.args = {}
Combat.version = 2
Combat.priority = constants.priorities.top.COMBAT

local ai = stonehearth.ai
return ai:create_compound_action(Combat)
   :execute('stonehearth:drop_carrying_now')
   :execute('stonehearth:set_posture', { posture = 'stonehearth:combat' })
   :execute('stonehearth:combat')
   :execute('stonehearth:unset_posture', { posture = 'stonehearth:combat' })
