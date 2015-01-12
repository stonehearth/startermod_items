local UnitControlService = class()

function UnitControlService:initialize()
   if not self._sv.unit_controllers then
      self._sv.unit_controllers = {}
   end
end

function UnitControlService:get_controller(player_id)
   local unit_controller = self._sv.unit_controllers[player_id]
   if not unit_controller then
      unit_controller = radiant.create_controller('stonehearth:unit_control:controller', player_id)
      assert(unit_controller)
      self._sv.unit_controllers[player_id] = unit_controller
      self.__saved_variables:mark_changed()
   end
   return unit_controller
end

function UnitControlService:get_controller_command(session, response)
   return { uri = self:get_controller(session.player_id) }
end

function UnitControlService:create_party_command(session, response)
   local party = self:get_controller(session.player_id)
                        :create_party()
   return { party = party }
end

return UnitControlService

