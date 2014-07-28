local MicroWorld = require 'lib.micro_world'
local BulletinTest = class(MicroWorld)

local count_1 = 1
local count_2 = 100

function BulletinTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self.__saved_variables = radiant.create_datastore()

   local footman = self:place_citizen(-15, -15, 'stonehearth:professions:footman', 'stonehearth:wooden_sword_proxy')
   local player_id = radiant.entities.get_player_id(footman)

   self._caravan_bulletin = stonehearth.bulletin_board:post_bulletin(player_id)
      :set_ui_view('StonehearthGenericBulletinDialog')
      :set_callback_instance(self)
      :set_data({
         title = 'A caravan has arrived!',
         message = 'A caravan of merchants has stopped by and wants to trade resources.',
         accepted_callback = "_on_accepted",
         declined_callback = "_on_declined",
      })

   stonehearth.bulletin_board:post_bulletin(player_id)
      :set_type('alert')
      :set_data({
         title = 'A goblin is stealing your wood!',
         zoom_to_entity = footman,
      })

   --radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._on_poll)
end

function BulletinTest:_on_accepted(param)
   -- will not actually be called in this test since we are not a real savable object
   local foo = param
end

function BulletinTest:_on_declined(param)
   -- will not actually be called in this test since we are not a real savable object
   local foo = param
end

function BulletinTest:_on_ok(param)
   -- will not actually be called in this test since we are not a real savable object
   local foo = param
end

local count = 1

function BulletinTest:_on_poll()
   -- self._caravan_bulletin:set_data({
   --    title = tostring(count)
   -- })
   -- count = count + 1

   stonehearth.bulletin_board:post_bulletin('player_1')
      :set_ui_view('StonehearthGenericBulletinDialog')
      :set_data({
         title = 'The carnival is here!',
      })
end

return BulletinTest
