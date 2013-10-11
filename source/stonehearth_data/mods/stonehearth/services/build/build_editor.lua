local BuildEditor = class()

function BuildEditor:__init()
   self._model = _radiant.client.create_data_store()
   self._model:set_controller(self)
end

function BuildEditor:get_model()
   return self._model
end

function BuildEditor:place_new_wall(session, response, wall_uri)
   local dispatch = {
      [_radiant.client.Input.MOUSE] = self._on_mouse_event,
      [_radiant.client.Input.KEYBOARD] = self._on_keyboard_event,
   }
   
   local proxy_wall = radiant.entities.create_entity(wall_uri)
   local proxy_wall_renderer = _radiant.client.create_render_entity(1, proxy_wall)
   
   self._input_capture = _radiant.client.capture_input()
   self._input_capture:on_input(function(e)
      if dispatch[e.type] then
         return dispatch[e.type](self, e, proxy_wall)
      end
      return false
   end)
   return { success=true }
end

function BuildEditor:_on_mouse_event(e, proxy_wall)
   local query = _radiant.client.query_scene(e.mouse.x, e.mouse.y)
   if query.location then
      proxy_wall:add_component('mob'):set_location_grid_aligned(query.location + query.normal)
   end
   return false
end

function BuildEditor:_on_keyboard_event(e)
end

function BuildEditor:place_new_wall_xxx(session, response)
   -- create a new "cursor entity".  this is the entity that will move around the
   -- screen to preview where the workbench will go.  these entities are called
   -- "authoring entities", because they exist only on the client side to help
   -- in the authoring of new content.
   local cursor_entity = radiant.entities.create_entity()
   local mob = cursor_entity:add_component('mob')
   mob:set_interpolate_movement(false)
   
   -- add a render object so the cursor entity gets rendered.
   local cursor_render_entity = _radiant.client.create_render_entity(1, cursor_entity)
   local node = h3dRadiantCreateStockpileNode(cursor_render_entity:get_node(), 'stockpile designation')

   -- change the actual game cursor
   local stockpile_cursor = _radiant.client.set_cursor('stonehearth:cursors:create_stockpile')

   local cleanup = function()
      stockpile_cursor:destroy()
      _radiant.client.destroy_authoring_entity(cursor_entity:get_id())
   end

   _radiant.client.select_xz_region()
      :progress(function (box)
            mob:set_location_grid_aligned(box.min)
            h3dRadiantResizeStockpileNode(node, box.max.x - box.min.x + 1, box.max.z - box.min.z + 1);
         end)
      :done(function (box)
            local size = {
               box.max.x - box.min.x + 1,
               box.max.z - box.min.z + 1,
            }
            _radiant.call('stonehearth:create_stockpile', box.min, size)
                     :done(function(r)
                           response:resolve(r)
                        end)
                     :always(function()
                           cleanup()
                        end)
         end)
      :fail(function()
            cleanup()
         end)
end

return BuildEditor
