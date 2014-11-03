local MicroWorld = require 'lib.micro_world'

local SoundTest = class(MicroWorld)

function SoundTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local e = self:place_item('stonehearth:large_oak_tree', 0, 0)
   radiant.effects.run_effect(e, '/stonehearth/data/effects/loop_sound_effect')
end

return SoundTest