local Array2DFns = {}

function Array2DFns.copy_vector(dst, src, dst_start, dst_inc, src_start, src_inc, length)
   local i
   local x = src_start
   local y = dst_start

   for i=1, length do
      dst[y] = src[x]
      x = x + src_inc
      y = y + dst_inc
   end
end

function Array2DFns.get_row_vector(dst, src, row_num, row_width, length)
   Array2DFns.copy_vector(dst, src, 1, 1, (row_num-1)*row_width+1, 1, length)
end

function Array2DFns.set_row_vector(dst, src, row_num, row_width, length)
   Array2DFns.copy_vector(dst, src, (row_num-1)*row_width+1, 1, 1, 1, length)
end

function Array2DFns.get_column_vector(dst, src, col_num, row_width, length)
   Array2DFns.copy_vector(dst, src, 1, 1, col_num, row_width, length)
end

function Array2DFns.set_column_vector(dst, src, col_num, row_width, length)
   Array2DFns.copy_vector(dst, src, col_num, row_width, 1, 1, length)
end

return Array2DFns

