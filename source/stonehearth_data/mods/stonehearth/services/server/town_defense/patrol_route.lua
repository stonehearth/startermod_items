PatrolRoute = class()

-- not al patrol routes will have an object of interest
function PatrolRoute:__init(points, patrollable_object)
   self.patrollable_object = patrollable_object
   self.points = points
   self.current_index = 1
end

function PatrolRoute:get_current_point()
   return self.points[self.current_index]
end

function PatrolRoute:on_last_point()
   return self.current_index >= #self.points
end

function PatrolRoute:advance_to_next_point()
   self.current_index = self.current_index + 1
end

return PatrolRoute
