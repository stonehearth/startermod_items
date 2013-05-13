local TeraGen = class()

local log = require 'radiant.core.log'
local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local check = require 'radiant.core.check'

function TeraGen:__init()
   self._types = {}
   
   md:register_msg_handler_instance('radiant.tg', self)
end

function TeraGen:register_terrain(factory, info)
   check:is_table(info)
   check:is_string(info.name)
   assert(not self._types[infoname])
   self._types[info.name] = factory
end


function create_sampler2d(o)
   local octaves = o.octaves and o.octaves or 4
   local persistance = o.persistance and o.persistance or 0.2
   local scale = o.scale and o.scale or 1
   local xoffset = o.xoffset and o.xoffset or 0
   local yoffset = o.yoffset and o.yoffset or 0
   
   return function(x, y)
      local total = 0
      local amplitude = 1
      local max_amplitude = 0
      local freq = scale
      x = x + xoffset
      y = y + yoffset
      for i = 1, octaves do
         total = total + (simplex.noise2d(x * freq, y * freq) * amplitude)
         freq = freq * 2
         max_amplitude = max_amplitude + amplitude
         amplitude = amplitude * persistance
      end
      -- normals from [-1, 1] to [0, 1]
      return (total / max_amplitude) + 1.0 / 2.0
   end
end

function debug_samplemap(m, block_size, filename)
   
   local size = m.size

   for x = 0, size-1 do
      for y = 0, size-1 do
         local v = m:get(x, y)
         cairo.set_source_rgb(cr, v, v, v)
         cairo.rectangle(cr, x * block_size, y * block_size, block_size, block_size)
         cairo.fill(cr)
      end
   end
end

local Layer = class()
function Layer:__init(name, size, lod, sampler)
   self._size = size
   self._lod = lod
   self._name = name
   self._sampler = sampler
end

function Layer:get_size()
   return self._size
end

function Layer:get_lod()
   return self._lod
end

function Layer:draw(cr)
   local range = self._size / self._lod
   local midpoint = self._lod / 2
   for x = 0, range-1 do
      for y = 0, range-1 do
         local value = self:sample(x * self._lod + midpoint, y * self._lod + midpoint)
         cairo.set_source_rgb(cr, value, value, value)
         cairo.rectangle(cr, x * self._lod, y * self._lod, self._lod, self._lod)
         cairo.fill(cr)
      end
   end
end

function Layer:sample(x, y)
   return self._sampler(x, y)
end

function Layer:write_png()
   local img_size = self._size
   local cs = cairo.image_surface_create(cairo.FORMAT_RGB24, img_size, img_size)
   local cr = cairo.create(cs)
   self:draw(cr)
   cairo.surface_write_to_png(cs, string.format("%s_%d.png", self._name, self._lod))
end

function Layer:get_heightmap(min, max)
   if not self._heightmap then
      local range = self._size / self._lod
      local midpoint = self._lod / 2

      self._heightmap = HeightMap(range, self._lod)
      for x = 0, range-1 do
         for y = 0, range-1 do
            local value = self:sample(x * self._lod + midpoint, y * self._lod + midpoint)
            value = min + (value * (max - min))
            self._heightmap:set(x, y, value)
         end
      end
   end
   return self._heightmap
end


local Config = {
   Foothills = {      
      Threshold = 0.35,
      Base = 1,
      StepHeight = 8,
      StepCount = 3,
   },
   Plains = {
      Threshold = 0.5,
      Base = 1,
      StepHeight = 3,
      StepCount = 1,
   },
   Foliage = {
      SamplerScale = 2,
      StepCount = 3,
      StepRadius = 8,
      Equation = "step",
      Sampler = { 
         scale = 2,
         xoffset = 121324,
         yoffset = 143255,
      }
   }, 
   Underbrush = {
      SamplerScale = 2, -- scale on the sampler
      StepCount = 2,
      StepRadius = 1,
      Equation = "step",
      Sampler = {
         scale = 8, -- scale on the density...
         xoffset = 23902,
         yoffset = 29035,
      }
   },   
}
-- -------------------------------------------------

local FoothillsLayer = class(Layer)

function FoothillsLayer:__init(elevation)
   self[Layer]:__init('foothills', elevation:get_size(), elevation:get_lod(), function(...) return self:sample(...) end);
   self._elevation = elevation
end

function FoothillsLayer:_add_layer_to_region(dst, rect, height)
   dst:add(Cube3(Point3(rect.min.x, -20, rect.min.y),
                Point3(rect.max.x, height - 1, rect.max.y),
                Terrain.TOPSOIL))
   dst:add(Cube3(Point3(rect.min.x, height - 1, rect.min.y),
                Point3(rect.max.x, height, rect.max.y),
                Terrain.GRASS))
end

function FoothillsLayer:_add_basic_blocks(r3, r2)
   for rect in r2:contents() do
      --log:info('adding rect to heightmap with tag %d', rect.tag)
      if rect.tag > 0 then
         --log:info('foothills step %d', rect.tag)
         local height = Config.Foothills.Base + (rect.tag * Config.Foothills.StepHeight)
         self:_add_layer_to_region(r3, rect, height);         
       end
   end
end

function FoothillsLayer:_add_detail_blocks(r3, r2)
   for slice = 1, Config.Foothills.StepCount do
      local layer = Region2()
      for rect in r2:contents() do
         if rect.tag == slice then
            layer:add_unique(rect)
         end
      end
      local el = region2_to_edge_list(layer)
      el:grow(1)
      el:fragment()

      local details = edge_list_to_region2(el, 1)
      for rect in details:contents() do
         local height = math.random(2, Config.Foothills.StepHeight / 2) -- xxx; config for 2!!
         local base = Config.Foothills.Base + ((slice - 1) * Config.Foothills.StepHeight)
         local top = base + height
         r3:add(Cube3(Point3(rect.min.x, base, rect.min.y),
                      Point3(rect.max.x, top - 1, rect.max.y),
                      Terrain.TOPSOIL_DETAIL))
         r3:add(Cube3(Point3(rect.min.x, top - 1, rect.min.y),
                      Point3(rect.max.x, top , rect.max.y),
                      Terrain.GRASS))
      end
   end
end

function FoothillsLayer:add_to_terrain(terrain)
   local r2 = Region2()
   local r3 = Region3()
      
   convert_heightmap_to_region2(self:get_heightmap(0, Config.Foothills.StepCount), r2)
   self:_add_basic_blocks(r3, r2)
   self:_add_detail_blocks(r3, r2)
   
   terrain:add_region(r3)
end

function FoothillsLayer:sample(x, y)
   local threshold = Config.Foothills.Threshold
   local e = self._elevation:sample(x, y)
   if e < threshold then
      return 0
   end
   --return simplex.map(e * h, 0, 1, "step", 32)
   e = (e - threshold) / (1 - threshold)
   return simplex.map(e, 0, 1, "step", Config.Foothills.StepCount + 1)
end

-- -------------------------------------------------

local PlainsLayer = class(Layer)

function PlainsLayer:__init(elevation)
   self[Layer]:__init('plains', elevation:get_size(), elevation:get_lod(), function(...) return self:sample(...) end);
   self._elevation = elevation
end

function PlainsLayer:add_to_terrain(terrain)
   local r2 = Region2()
   local r3 = Region3()
   convert_heightmap_to_region2(self:get_heightmap(0, Config.Plains.StepCount), r2)
   for rect in r2:contents() do
      local height = Config.Plains.Base + (rect.tag * Config.Plains.StepHeight)
      r3:add(Cube3(Point3(rect.min.x, -20, rect.min.y),
                   Point3(rect.max.x, height - 1, rect.max.y),
                   Terrain.TOPSOIL))      
      r3:add(Cube3(Point3(rect.min.x, height - 1, rect.min.y),
                   Point3(rect.max.x, height, rect.max.y),
                   Terrain.PLAINS))
   end
   terrain:add_region(r3)
end

function PlainsLayer:sample(x, y)
   local threshold = Config.Plains.Threshold
   local e = self._elevation:sample(x, y)
   if e > threshold then
      return 1
   end
   --return simplex.map(e * h, 0, 1, "step", 32)
   e = e / threshold
   return simplex.map(e, 0, 1, "step", Config.Plains.StepCount + 1)
end

-- -------------------------------------------------

local ElevationLayer = class(Layer)

function ElevationLayer:__init(size, lod)
   self[Layer]:__init('elevation', size, lod, function(...) return self:sample(...) end )
   local options = {
      octaves = 2,
      persistance = 0.3,
      scale = 1 / size,
      xoffset = 123245,
      yoffset = 1435
   }   
   self._noise_sampler = create_sampler2d(options)
end

function ElevationLayer:sample(x, y)
   return self._noise_sampler(x, y)
   --return (x * y) / (size * size)
end

-- -------------------------------------------------

local FoliageLayer = class(Layer)

function FoliageLayer:__init(elevation, o)
   local size = elevation:get_size();
   
   assert(o)
   assert(o.SamplerScale)
   assert(o.StepCount)
   
   self._foliage_options = o
   self._lod = elevation:get_lod()
   
   self[Layer]:__init('foliage',
                      elevation:get_size(),
                      elevation:get_lod() * o.SamplerScale, function(...) return self:sample(...) end )
   local sampler_option = function(name, default)
      local option = o.Sampler and o.Sampler[name]
      return option and option or default
   end
   local options = {
      octaves = 2,
      persistance = 0.3,
      scale = sampler_option('scale', 1) / size,
      xoffset = sampler_option('xoffset', 0),
      yoffset = sampler_option('yoffset', 0),
   }
   self._elevation = elevation
   self._density_sampler = create_sampler2d(options)
end

function FoliageLayer:sample(x, y)
   local density = self._density_sampler(x, y)
   local elevation = self._elevation:sample(x, y)
   local intensity = density * (1 - elevation)
   
   local eq = self._foliage_options.Equation
   return simplex.map(intensity, 0, 1, eq, self._foliage_options.StepCount + 1);
   --return (x * y) / (size * size)
end

-- -------------------------------------------------

function create_voronoi_map(radius, size)
   local relax_iterations = 8
   local point_count = (size / radius) * (size / radius)
   
   math.randomseed(2)
   
   local voronoi_map = VoronoiMap(size, size)   
   for i = 1, point_count do
      voronoi_map:add_point(math.random(0, size - 1), math.random(0, size - 1));
   end
   voronoi_map:generate(relax_iterations)
   return voronoi_map
end

-- ----

local FoliageMapper = class()

function FoliageMapper:__init(elevation)
   self._size = elevation:get_size()
   -- big foliage layer
   self._foliage = FoliageLayer(elevation, Config.Foliage)
   self._point_map = create_voronoi_map(15, self._size)     
   self._point_map:distribute_by_heightmap(self._foliage:get_heightmap(0, Config.Foliage.StepCount), Config.Foliage.StepRadius)
   
   -- underbrush layer
   self._underbrush = FoliageLayer(elevation, Config.Underbrush)
   
   log:info('sup...')
   --self._point_map:distribute_by_heightmap()
   --self._medium_map = ScatterPlot(40, self._size)
   --self._large_map = ScatterPlot(60, self._size)
   
end

function FoliageMapper:add_to_terrain(terrain)
   self:_add_trees_to_terrain(terrain)
   self:_add_undergrowth_to_terrain(terrain)
end

function FoliageMapper:_add_trees_to_terrain(terrain)
   local trees = {
      'stonehearth.resources.trees.small_oak_tree',
      'stonehearth.resources.trees.medium_oak_tree',
      'stonehearth.resources.trees.large_oak_tree'
   }
   self:_add_items_to_terrain(terrain, self._point_map, trees)
end

function FoliageMapper:_add_undergrowth_to_terrain(terrain)
   local undergrowth = {
      'flower_pink',
   }
   local lod = self._foliage:get_lod()
   local hm = self._underbrush:get_heightmap(0, Config.Underbrush.StepCount)
   for y = 0, hm:size() - 1 do
      for x = 0, hm:size() - 1 do
         local value = hm:get(x, y)
         if value > Config.Underbrush.StepCount - 1 then
            local density = Config.Underbrush.StepCount - value + 1
            for i = 0, math.random(density * lod / 2) do
               local pos_x = (x * lod) + math.random(lod)
               local pos_y = (y * lod) + math.random(lod)
               local clutter = om:create_entity(undergrowth[math.random(#undergrowth)])
               om:place_on_terrain(clutter, RadiantIPoint3(pos_x, 1, pos_y))
               om:get_component(clutter, 'mob'):turn_to(math.random(0, 360))
            end
         end
      end
   end
end

function FoliageMapper:_add_items_to_terrain(terrain, pointmap, items)
   -- xxx: the step count shouldn't need to be passed in every time...
   for site in pointmap.sites do
      local entity_name = items[site.tag]
      if entity_name then
         local tree = om:create_entity(entity_name)
         om:place_on_terrain(tree, RadiantIPoint3(site.pos.x, 1, site.pos.y))
         om:get_component(tree, 'mob'):turn_to(math.random(0, 3) * 90)
      end
   end
end

function FoliageMapper:draw_sites(cr, radius, pointmap)
   for site in pointmap.sites do
      cairo.arc(cr, site.pos.x, site.pos.y, radius, 0, 2 * math.pi)
      cairo.fill(cr)
   end
end

function FoliageMapper:draw(cr, layer, pointmap)
   layer:draw(cr)
   cairo.set_source_rgb(cr, 0, 1, 0)
   if pointmap then
      self:draw_sites(cr, 2, pointmap)
   end
   --self:draw_sites(cr, self._medium_map, 4)
   --self:draw_sites(cr, self._large_map, 6)
end

function FoliageMapper:_write_layer(name, layer, sites)
   local cs = cairo.image_surface_create(cairo.FORMAT_RGB24, self._size, self._size)
   local cr = cairo.create(cs)
   self:draw(cr, layer, sites)
   cairo.surface_write_to_png(cs, name)
end

function FoliageMapper:write_png()
   self:_write_layer('foliage.png', self._foliage, self._point_map)  
   self:_write_layer('underbrush.png', self._underbrush, self._underbrush_pointmap)
end

function TeraGen:create()
--[[
   log:info('new game!')
   local size = 32
   local height = 16
   local game_bounds = RadiantBounds3(RadiantIPoint3(-size, -height, -size), RadiantIPoint3(size, height, size))
   
   -- Create the plains...
   local plains = game_bounds;
   plains.max.y = 1;
   self._types['plains']():create_layer(plains)
   
   -- Put a forest in the top right corner
   local forest_region = Region2(Rect2(Point2(0, 0), Point2(size, size)))
   self._types['forest']():create_layer(forest_region, 1)
   ]]
   --local lod, size = 4, 32
   local lod, size = 8, 512
   local e = ElevationLayer(size, lod)
   
   local foliage = FoliageMapper(e)
   local foothills = FoothillsLayer(e)
   local plains = PlainsLayer(e)
   
   e:write_png()
   foliage:write_png()
   foothills:write_png()
   plains:write_png()

   local terrain = om:get_terrain()
   if true then
      foothills:add_to_terrain(terrain)
      plains:add_to_terrain(terrain)
      foliage:add_to_terrain(terrain)
      log:info('done!')
   elseif false then
      local r = Region3()
      for x = -100, 100, 20 do
         for y = -100, 100, 20 do
            r:add(Cube3(Point3(x, 0, y), Point3(x+2, 8, y+2), Terrain.GRASS))
         end
      end
      terrain:add_region(r)
   else
      local r = Region3()
      --r:add(Cube3(Point3(-10, 0, -10), Point3(10, 8, 10), Terrain.TOPSOIL))
      --r:add(Cube3(Point3(-10, 8, -10), Point3(10, 9, 10), Terrain.GRASS))
      --r:add(Cube3(Point3(-15, 8, -15), Point3(-6, 9, -6), Terrain.GRASS))
      --r:add(Cube3(Point3(6, 8, 6), Point3(15, 9, 15), Terrain.GRASS))
      --r:add(Cube3(Point3(0, 8, -15), Point3(15, 9, 0), Terrain.GRASS))
      
      -- non-intersecting cube test...
      --r:add(Cube3(Point3(1, 0, 1), Point3(4, 8, 4), Terrain.GRASS))
      --r:add(Cube3(Point3(6, 0, 6), Point3(10, 8, 10), Terrain.GRASS))

      -- completely overlapping-intersecting cube test...
      --r:add(Cube3(Point3(6, 0, 6), Point3(10, 8, 10), Terrain.GRASS))
      --r:add(Cube3(Point3(5, 0, 5), Point3(11, 8, 11), Terrain.GRASS))

      local s = 10
      -- box test
      if false then
         r:add(Cube3(Point3(-s, 0, -s), Point3(s, 1, s), Terrain.GRASS))
      end
      
      -- L overlap test
      if false then
         r:add(Cube3(Point3(0, 0, 0), Point3(s, 1, s/2), Terrain.GRASS))
         r:add(Cube3(Point3(0, 0, 0), Point3(s/2, 1, s), Terrain.GRASS))
      end

      -- + overlap test
      if false then
         r:add(Cube3(Point3(-s, 0, -s/2), Point3(s, 1, s/2), Terrain.GRASS))
         r:add(Cube3(Point3(-s/2, 0, -s), Point3(s/2, 1, s), Terrain.GRASS))
      end
      
      -- + and [] test (still buggy)
      for x = 0, 4 do
         for y = 0, 4 do
            local skip = (x == 1 and y == 1) or (x == 1 and y == 3) or (x == 3 and y == 1) or (x == 3 and y == 3)
            if not skip then
               local p0 = Point3(x * s, 0, y * s)
               local p1 = Point3((x + 1) * s, 1, (y + 1) * s)
               r:add(Cube3(p0, p1, Terrain.GRASS))
            end
         end
      end
      
      terrain:add_region(r)
   end
   
   print('yay!')   
end

return TeraGen()
