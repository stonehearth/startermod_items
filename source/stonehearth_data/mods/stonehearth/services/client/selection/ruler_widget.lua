local Mesh = _radiant.csg.Mesh
local Vertex = _radiant.csg.Vertex
local Point3 = _radiant.csg.Point3
local Color4 = _radiant.csg.Color4

local LINE_WIDTH = 0.15
local ARROW_WIDTH = 0.8
local ARROW_HEIGHT = 0.5

local RulerWidget = class()

function RulerWidget:__init()
   self._padding = 1;
   self._margin = 1;
   self._line_color = Color4(0, 0, 0, 128)
   self._meshNormal = Point3(0, 1, 0)
   self._hidden = false
end

function RulerWidget:destroy()
   self:_destroy_render_nodes()
end

function RulerWidget:set_points(start, finish, normal, label)
   self._normal = normal
   self._label = label
   self._start = start + normal + Point3(-0.5, 0.01, -0.5)
   self._finish = finish + normal + Point3(0.5, 0.01, 0.5)
   if normal.z < 0 then
      local offset = self:_get_height() - 1
      self._start.z = self._start.z - offset
      self._finish.z = self._finish.z - offset
   end
   self:_recreate_render_node(self._label);
end

function RulerWidget:set_color(color)
   self._line_color = color
end

function RulerWidget:hide()
   if not self._hidden then
      self:_destroy_render_nodes()
      self._hidden = true
   end
end

function RulerWidget:show()
   if self._hidden then
      self._hidden = false
      self:_recreate_render_node(self._label);
   end
end

function RulerWidget:_destroy_render_nodes()
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
   if self._text_node then
      self._text_node:destroy()
      self._text_node = nil
   end   
end

function RulerWidget:_transform_point(pt)
   if self._normal.z < 0 then
      pt.x = self:_get_width() - pt.x     -- mirror about x
      pt.z = self:_get_height() - pt.z
   elseif self._normal.x > 0 then
      pt.x, pt.z = pt.z, pt.x             -- flip
   elseif self._normal.x < 0 then
      pt.x, pt.z = pt.z, pt.x             -- flip and mirror
      pt.x = -pt.x + 1
   end
   return pt
end

function RulerWidget:_add_quad(mesh, x, z, w, h, color)
   local function push(x, y, z)
      local point = self:_transform_point(Point3(x, y, z))
      point = point + self._start
      mesh:add_vertex(Vertex(point, self._meshNormal, color))
   end
   
   local y = 0
   local ioffset = mesh:get_vertex_count()
   if self._normal.x > 0 then
      push(x + w, y, z)
      push(x + w, y, z + h)
      push(x, y, z + h)
      push(x, y, z)
   else   
      push(x, y, z)
      push(x, y, z + h)
      push(x + w, y, z + h)
      push(x + w, y, z)
   end
   
   mesh:add_index(ioffset)      -- first triangle
   mesh:add_index(ioffset + 1)
   mesh:add_index(ioffset + 2)
   mesh:add_index(ioffset)      -- second triangle
   mesh:add_index(ioffset + 2)
   mesh:add_index(ioffset + 3)
end

function RulerWidget:_add_triangle(mesh, p0, p1, p2, color)
   local function push(pt)
      local point = self:_transform_point(Point3(pt.x, pt.y, pt.z))
      point = point + self._start
      mesh:add_vertex(Vertex(point, self._meshNormal, color))
   end
   
   local y = 0
   local ioffset = mesh:get_vertex_count()
   if self._normal.x > 0 then
      push(p2)
      push(p1)
      push(p0)
   else   
      push(p0)
      push(p1)
      push(p2)
   end
   
   mesh:add_index(ioffset)      -- first triangle
   mesh:add_index(ioffset + 1)
   mesh:add_index(ioffset + 2)
end

function RulerWidget:_get_width()
   if self._normal.x == 0 then
      return math.abs(self._finish.x - self._start.x)
   end
   return math.abs(self._finish.z - self._start.z)
end

function RulerWidget:_get_height()
   return self._padding + self._margin
end

function RulerWidget:_recreate_render_node(label)
   self:_destroy_render_nodes()

   if self._hidden then
      return
   end

   local w = self:_get_width()
   local h = self:_get_height()
   if w <= 1 then
      return
   end
   
   local mesh = Mesh()

   -- the line on the left and right
   self:_add_quad(mesh, 0, 0, LINE_WIDTH, h, self._line_color)
   self:_add_quad(mesh, w - LINE_WIDTH, 0, LINE_WIDTH, h, self._line_color)
   
   -- line through the center
   self:_add_quad(mesh, LINE_WIDTH + ARROW_WIDTH,
                        self._padding - (LINE_WIDTH / 2),
                        w - ((LINE_WIDTH + ARROW_WIDTH) * 2),
                        LINE_WIDTH,
                        self._line_color)
   -- left arrow
   self:_add_triangle(mesh, Point3(LINE_WIDTH, 0, self._padding),
                            Point3(LINE_WIDTH + ARROW_WIDTH, 0, self._padding + (ARROW_HEIGHT / 2)),
                            Point3(LINE_WIDTH + ARROW_WIDTH, 0, self._padding - (ARROW_HEIGHT / 2)),
                            self._line_color)
   -- right arrow
   self:_add_triangle(mesh, Point3(w - LINE_WIDTH, 0, self._padding),
                            Point3(w - LINE_WIDTH- ARROW_WIDTH, 0, self._padding - (ARROW_HEIGHT / 2)),
                            Point3(w - LINE_WIDTH- ARROW_WIDTH, 0, self._padding + (ARROW_HEIGHT / 2)),
                            self._line_color)
   

   self._render_node = _radiant.client.create_mesh_node(1, mesh)
   self._render_node:set_can_query(false)
                    :set_material('materials/transparent.material.xml')

   if label then
      self._text_node = _radiant.client.create_text_node(1, label)
      self._text_node:set_position((self._start + self._finish) / 2)
   end
end

return RulerWidget
