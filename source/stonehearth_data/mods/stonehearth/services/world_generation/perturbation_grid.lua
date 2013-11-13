
local PerturbationGrid = class()

function PerturbationGrid:__init(source_map_width, source_map_height, grid_spacing)
   self.grid_spacing = grid_spacing
   self.perturbation_map_width = math.floor(source_map_width / grid_spacing)
   self.perturbation_map_height = math.floor(source_map_height / grid_spacing)
   
   local remainder_x = source_map_width % grid_spacing
   local remainder_y = source_map_height % grid_spacing

   self._margin_left_x = math.floor(remainder_x*0.5)
   self._margin_right_x = remainder_x - self._margin_left_x
   self._margin_top_y = math.floor(remainder_y*0.5)
   self._margin_bottom_y = remainder_y - self._margin_top_y
end

function PerturbationGrid:get_dimensions()
   return self.perturbation_map_width, self.perturbation_map_height
end

function PerturbationGrid:get_cell_bounds(i, j)
   local grid_spacing = self.grid_spacing
   local width = grid_spacing
   local height = grid_spacing
   local x, y

   x = (i-1)*grid_spacing + 1

   if i == 1 then
      width = width + self._margin_left_x
   else
      x = x + self._margin_left_x
   end

   if i == self.perturbation_map_width then
      width = width + self._margin_right_x
   end


   y = (j-1)*grid_spacing + 1

   if j == 1 then
      height = height + self._margin_top_y
   else
      y = y + self._margin_top_y
   end

   if j == self.perturbation_map_height then
      height = height + self._margin_bottom_y
   end

   return x, y, width, height
end

function PerturbationGrid:get_perturbed_coordinates(i, j, margin_size)
   if margin_size*2 >= self.grid_spacing then
      margin_size = math.floor((self.grid_spacing-1)*0.5)
   end

   local x, y, width, height = self:get_cell_bounds(i, j)
   local cell_x = math.random(margin_size, width-1-margin_size)
   local cell_y = math.random(margin_size, height-1-margin_size)
   return x + cell_x, y + cell_y
end

return PerturbationGrid
