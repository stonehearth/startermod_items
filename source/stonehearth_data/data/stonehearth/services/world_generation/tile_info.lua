local TileInfo = class()

function TileInfo:__init()
end

function TileInfo:clear()
   self.forced_value = nil
   self.mean = nil
   self.std_dev = nil
   self.mean_weight = nil
   self.std_dev_weight = nil
end

function TileInfo:blend_stats_from(tile)
   if self.forced_value then return end
   if tile.forced_value then
      self:clear()
      self.forced_value = tile.forced_value
      return
   end

   local sum_mean_weights = self.mean_weight + tile.mean_weight
   self.mean = (self.mean*self.mean_weight + tile.mean*tile.mean_weight) / sum_mean_weights
   self.mean_weight = sum_mean_weights

   local sum_std_dev_weights = self.std_dev_weight + tile.std_dev_weight
   self.std_dev = (self.std_dev*self.std_dev_weight + tile.std_dev*tile.std_dev_weight) / sum_std_dev_weights
   self.std_dev_weight = sum_std_dev_weights
end

return TileInfo
