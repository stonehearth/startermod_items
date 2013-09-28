local object_tracker = require 'services.object_tracker.object_tracker_service'

local ObjectTrackerCallHandler = class()

function format_response(tracker)
   return {
      tracker = tracker:get_data_store()
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


return ObjectTrackerCallHandler
