local constants = require 'constants'
local log = radiant.log.create_logger('combat')


-- Key nodes in the combat AI tree
--
-- Activities are in quotes
-- Actions are underneath activities
-- Priorities are in parenthesis
-- 
-- 'stonehearth:top'
-- combat(100)
--    'stonehearth:combat'
--    attack_distpatcher(10)
--       'stonehearth:combat:attack'
--       attack_melee(1)
--          'stonehearth:combat:attack_melee_adjacent'
--          attack_melee_adjacent(1)
--       attack_ranged(2)
--       attack_spell(3)
--    defend_dispatcher(10)
--       'stonehearth:combat:defend'
--       defend_melee(1)
--       defend_ranged(1)
--    combat_idle_dispatcher(1)
--       'stonehearth:combat:idle'
--       combat_idle_shuffle(2)
--       combat_idle_ready(1)
--    panic_dispatcher(20)
--       'stonehearth:combat:panic'
--       flee(1)
-- compelled_behavior_dispatcher(9999999)
--    'stonehearth:compelled_behavior'
--    hit_stun(100)


local Combat = class()
Combat.name = 'combat'
Combat.does = 'stonehearth:top'
Combat.args = {}
Combat.version = 2
Combat.priority = constants.priorities.top.COMBAT

local ai = stonehearth.ai
return ai:create_compound_action(Combat)
   :execute('stonehearth:combat:find_target')
   :execute('stonehearth:drop_carrying_now')
   :execute('stonehearth:set_posture', { posture = 'combat' })
   :execute('stonehearth:combat', { enemy = ai.BACK(3).target })
   :execute('stonehearth:unset_posture', { posture = 'combat' })
