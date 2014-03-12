local constants = require 'constants'

local AmbientPetDispatcher = class()
AmbientPetDispatcher.name = 'be a pet'
AmbientPetDispatcher.does = 'stonehearth:top'
AmbientPetDispatcher.args = {}
AmbientPetDispatcher.version = 2
AmbientPetDispatcher.priority = constants.priorities.top.AMBIENT_PET_BEHAVIOR

local ai = stonehearth.ai
return ai:create_compound_action(AmbientPetDispatcher)
         :execute('stonehearth:ambient_pet_behavior')
