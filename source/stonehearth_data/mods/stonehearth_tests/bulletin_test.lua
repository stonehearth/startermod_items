local MicroWorld = require 'lib.micro_world'
local BulletinTest = class(MicroWorld)

local count_1 = 1
local count_2 = 100

function BulletinTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local footman = self:place_citizen(-15, -15, 'stonehearth:professions:footman', 'stonehearth:wooden_sword')

   self._caravan_bulletin = stonehearth.bulletin_board:post_bulletin('player_1')
      :set_config('stonehearth:bulletins:caravan')
      :set_callback_instance(self)
      :set_data({
         title = 'Caravan has arrived!',
         message = 'A caravan of merchants has stopped by and wants to trade resources.',
      })


   stonehearth.bulletin_board:post_bulletin('player_1')
      :set_config('stonehearth:bulletins:goblin_thief')
      :set_data({
         title = 'A goblin is stealing your wood!',
         message = 'A goblin thief is making off with your wood. Stop him!',
         show_me_entity = footman,
      })
end

return BulletinTest
