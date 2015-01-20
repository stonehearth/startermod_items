
local PartyEditorService = class()

function PartyEditorService:initialize()
end

function PartyEditorService:set_attack_order_command(session, response, party)
   local cursor_entity = radiant.entities.create_entity('stonehearth:attack_order_banner')

   cursor_entity:add_component('render_info')
                     :set_material('materials/ghost_item.json')

   stonehearth.selection:select_location()
      :set_rotation_disabled(false)
      :set_cursor_entity(cursor_entity)
      :set_filter_fn(function (result, selector)
            local normal = result.normal:to_int()
            local location = result.brick:to_int()

            if normal.y ~= 1 then
               return stonehearth.selection.FILTER_IGNORE
            end
            if not radiant.terrain.is_standable(location) then
               return stonehearth.selection.FILTER_IGNORE
            end
            return true
         end)
      :done(function(selector, location, rotation)
            _radiant.call_obj(party, 'create_attack_order_command', location, rotation)
                        :always(function()
                              selector:destroy()
                           end)
         end)
      :fail(function(selector)
            selector:destroy()
            response:reject('no location')
         end)
      :always(function()
            radiant.entities.destroy_entity(cursor_entity)
         end)
      :go()   
end

return PartyEditorService
