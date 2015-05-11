local Color4 = _radiant.csg.Color4

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

local log = radiant.log.create_logger('build_editor')

local DeleteStructureEditor = class()

function DeleteStructureEditor:__init(build_service)
   self._build_service = build_service
end

function DeleteStructureEditor:destroy()
   stonehearth.selection:register_tool(self, false)

   if self._indicator then
      self._indicator:destroy()
      self._indicator = nil
   end
end

function DeleteStructureEditor:go(response)
   stonehearth.selection:select_entity_tool()
      :set_cursor('stonehearth:cursors:delete_structure')
      :set_filter_fn(function(result)
            local entity = result.entity
            local fc = entity:get_component('stonehearth:fixture_fabricator')
            if fc then
               return true
            end
            return stonehearth.selection.FILTER_IGNORE
         end)
      :progress(function(selector, entity)
            self:_switch_to_target(entity)
         end)
      :done(function(selector, entity)
            if not entity then
               response:resolve('done')
               self:destroy()
            end

            _radiant.call_obj(self._build_service, 'delete_structure_command', entity)
               :done(function(r)
                     response:resolve(r)
                  end)
               :fail(function(r)
                     response:reject(r)
                  end)
               :always(function()
                     self:destroy()
                  end)
         end)
      :fail(function(selector)
            --[[
            log:detail('failed to select building')
            response:reject('failed')
            ]]
            self:destroy()
         end)
      :go()

   return self
end

function DeleteStructureEditor:_switch_to_target(entity)
   if entity ~= self._current_target then
      local color = Color4(255, 0, 0, 128)
      local model = Region3(Cube3(Point3(-1, -1, -1), Point3(1, 1, 1)))

      local location = radiant.entities.get_world_grid_location(entity)
      model:translate(location)

      if self._indicator then
         self._indicator:destroy()
         self._indicator = nil
      end
      self._indicator = _radiant.client.create_region_outline_node(H3DRootNode,
                                                                   model,
                                                                   color,
                                                                   color,
                                                                   'materials/transparent.material.json')
   end
end

return DeleteStructureEditor
