local tracker_manager = require 'lib.object_tracker.tracker_manager'

local ObjectTrackerCallHandler = class()

function format_response(tracker)
   return {
      tracker = tracker:get_data_store()
   }
end

function ObjectTrackerCallHandler:get_worker_tracker(session, response)
   local tracker = tracker_manager:get_worker_tracker(session.faction)
   return format_response(tracker)
end

function ObjectTrackerCallHandler:get_placable_items_tracker(session, response)
   local tracker = tracker_manager:get_placable_items_tracker(session.faction)
   return format_response(tracker)
end


return ObjectTrackerCallHandler
