local object_tracker = stonehearth.object_tracker

local ObjectTrackerCallHandler = class()

function format_response(tracker)
   return {
      tracker = tracker
   }
end

function ObjectTrackerCallHandler:get_worker_tracker(session, response)
   local tracker = object_tracker:get_worker_tracker(session.faction)
   return format_response(tracker)
end

function ObjectTrackerCallHandler:get_placable_items_tracker(session, response)
   local tracker = object_tracker:get_placable_items_tracker(session.faction)
   return format_response(tracker)
end

function ObjectTrackerCallHandler:get_resource_tracker(session, response)
   local tracker = object_tracker:get_resource_tracker(session.faction)
   return format_response(tracker)
end


return ObjectTrackerCallHandler
