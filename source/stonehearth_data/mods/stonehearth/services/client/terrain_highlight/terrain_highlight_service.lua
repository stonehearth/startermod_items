local OreBlockRenderer = require 'renderers.ore_block.ore_block_renderer'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('terrain_highlight')

TerrainHighlightService = class()

function TerrainHighlightService:__init()
   self._selection_radius = 8

   self._kind_to_entity_map = {
      copper_ore = radiant.entities.create_entity('stonehearth:terrain:ui:copper_block'),
      tin_ore = radiant.entities.create_entity('stonehearth:terrain:ui:tin_block'),
      iron_ore = radiant.entities.create_entity('stonehearth:terrain:ui:iron_block'),
      coal = radiant.entities.create_entity('stonehearth:terrain:ui:coal_block'),
      silver_ore = radiant.entities.create_entity('stonehearth:terrain:ui:silver_block'),
      gold_ore = radiant.entities.create_entity('stonehearth:terrain:ui:gold_block')
   }

   -- self._id_to_entity_map = {}
   -- for kind, entity in pairs(self._kind_to_entity_map) do
   --    local id = entity:get_id()
   --    self._id_to_entity_map[id] = entity
   -- end

   self._empty_region = Region3()
   self._far_away = Point3(radiant.math.MAX_INT32, radiant.math.MAX_INT32, radiant.math.MAX_INT32)
end

function TerrainHighlightService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.initialized = true
   else
   end

   self:_create_renderers()

   self:_listen_to_hilighted_changed()

   stonehearth.hilight:add_custom_highlight(function(result)
         local id = result.entity:get_id()

         if id == 1 then
            local brick = result.brick
            local kind = radiant.terrain.get_block_kind_at(brick)
            local ore_entity = self._kind_to_entity_map[kind]

            if ore_entity then
               radiant.entities.move_to(ore_entity, brick)

               -- enable complex shapes when the z-level of the selection outline is fixed
               --local shape = self:_get_selection_shape(brick, kind)
               local shape = Region3(Cube3(Point3.zero, Point3.one))

               local renderer = self._renderers[kind]
               renderer:set_region(shape)

               return ore_entity
            end
         end

         return nil
      end)
end

function TerrainHighlightService:destroy()
   self:_unlisten_to_hilighted_changed()
   self:_destroy_renderers()
end

function TerrainHighlightService:_get_selection_shape(location, kind)
   local radius = self._selection_radius
   local tag = radiant.terrain.get_block_tag_from_kind(kind)
   local cube = Cube3(
         Point3(-radius, -radius, -radius),
         Point3(radius+1, radius+1, radius+1)
      )

   -- to world space for terrain intersection
   cube:translate(location)

   local intersection = radiant.terrain.intersect_cube(cube)
   local ore_region = Region3()

   -- select only the cubes that are of the proper ore type
   for cube in intersection:each_cube() do
      if cube.tag == tag then
         ore_region:add_unique_cube(cube)
      end
   end

   -- back to local space for rendering
   ore_region:translate(-location)

   return ore_region
end

function TerrainHighlightService:_create_renderers()
   self._renderers = {}

   -- The authoring root entity doesn't create render entities for its children,
   -- so explicity do it here. They will exist as long as the entities exist.
   for kind, ore_entity in pairs(self._kind_to_entity_map) do
      local render_entity = _radiant.client.create_render_entity(1, ore_entity)
      self._renderers[kind] = OreBlockRenderer(render_entity)
   end
end

function TerrainHighlightService:_destroy_renderers()
   for kind, renderer in pairs(self._renderers) do
      -- the render entities will be destroyed in C++ when the entity is destoyed
      renderer:destroy()
   end

   self._renderers = {}
end

function TerrainHighlightService:_listen_to_hilighted_changed()
   self._highlight_listeners = {}

   for kind, ore_entity in pairs(self._kind_to_entity_map) do
      local listener = radiant.events.listen(ore_entity, 'stonehearth:hilighted_changed', function()
            if stonehearth.hilight:get_hilighted() ~= ore_entity then
               -- too much trouble to reparent the render entity, so move it far away
               -- so it doesn't confuse anyone in debug view
               radiant.entities.move_to(ore_entity, self._far_away)
               local renderer = self._renderers[kind]
               renderer:set_region(self._empty_region)
            end
         end)
      self._highlight_listeners[kind] = listener
   end
end

function TerrainHighlightService:_unlisten_to_hilighted_changed()
   for kind, listener in pairs(self._highlight_listeners) do
      listener:destroy()
   end
   self._highlight_listeners = {}
end

return TerrainHighlightService
