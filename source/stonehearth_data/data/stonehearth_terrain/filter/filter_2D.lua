
local Array2DFns = radiant.mods.require('/stonehearth_terrain/filter/array_2D_fns.lua')

function filter_2D(src, src_width, src_height, filter_func)
   local i, j
   local vec = {}

   -- horizontal
   for i=1, src_height, 1 do
      Array2DFns.get_row_vector(vec, src, i, src_width, src_width)
      filter_func(vec, src_width)
      Array2DFns.set_row_vector(src, vec, i, src_width, src_width)
   end

   -- vertical
   for j=1, src_width, 1 do
      Array2DFns.get_column_vector(vec, src, j, src_width, src_height)
      filter_func(vec, src_height)
      Array2DFns.set_column_vector(src, vec, j, src_width, src_height)
   end
end

return filter_2D

