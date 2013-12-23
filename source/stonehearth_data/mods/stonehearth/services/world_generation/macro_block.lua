local MacroBlock = class()

function MacroBlock:__init()
end

function MacroBlock:clear()
   self.forced_value = nil
   self.mean = nil
   self.std_dev = nil
   self.mean_weight = nil
   self.std_dev_weight = nil
end

function MacroBlock:blend_stats_from(mb)
   if self.forced_value then return end
   if mb.forced_value then
      self:clear()
      self.forced_value = mb.forced_value
      return
   end

   local sum_mean_weights = self.mean_weight + mb.mean_weight
   self.mean = (self.mean*self.mean_weight + mb.mean*mb.mean_weight) / sum_mean_weights
   self.mean_weight = sum_mean_weights

   local sum_std_dev_weights = self.std_dev_weight + mb.std_dev_weight
   self.std_dev = (self.std_dev*self.std_dev_weight + mb.std_dev*mb.std_dev_weight) / sum_std_dev_weights
   self.std_dev_weight = sum_std_dev_weights
end

return MacroBlock
