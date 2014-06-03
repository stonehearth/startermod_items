local MicroWorld = require 'lib.micro_world'
local BulletinTest = class(MicroWorld)

local count_1 = 1
local count_2 = 100

function BulletinTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local footman = self:place_citizen(-15, -15, 'stonehearth:professions:footman', 'stonehearth:wooden_sword')

   self:at(2000,
      function ()
         self._caravan_bulletin = stonehearth.bulletin_board:post_bulletin('player_1')
            :set_config('stonehearth:bulletins:caravan')
            :set_callback_instance(self)
            :set_data({
               title = 'Caravan has arrived!',
               message = 'A caravan of merchants has stopped by and wants to trade resources.',
            })
      end
   )

   -- self:at(2500,
   --    function ()
   --       -- pretend the user accepted the dialog
   --       -- in javascript this would be:
   --       --    radiant.call_obj(source, 'accepted', args);
   --       local source = self._caravan_bulletin:get_callback_instance()
   --       source:accepted(314)
   --    end
   -- )

   self:at(4000,
      function ()
         stonehearth.bulletin_board:post_bulletin('player_1')
            :set_config('stonehearth:bulletins:goblin_thief')
            :set_data({
               title = 'A goblin is stealing your wood!',
               message = 'A goblin thief is making off with your wood. Stop him!',
               show_me_entity = footman,
            })
      end
   )
end

function BulletinTest:accepted(param)
   local foo = param
end

return BulletinTest
