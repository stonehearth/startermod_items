/***
 * A manager for handling portrait requests for entities.
 ***/
var StonehearthPortraitManager;

(function () {
   StonehearthPortraitManager = SimpleClass.extend({

      init: function() {
         this._portraitRequests = {};
         this._nextRequestId = 0;
         this._lastProcessedRequestId = 0;
         this._currentlyProcessingRequest = false;
      },

      requestPortrait: function(scene, callback) {
         var requestId = this._nextRequestId;
         this._nextRequestId ++;
         var portraitRequest = {
            "__id" : requestId,
            "scene" : scene,
            "callback" : callback
         };
         this._portraitRequests[requestId] = portraitRequest;
         this._onRequestsModified();
         return requestId;
      },

      cancelRequest: function(requestId) {
         if (this._currentlyProcessingRequest
            && (requestId == this._lastProcessedRequestId)) {
            return false;
         }
         if (!(requestId in this._portraitRequests)) {
            return false;
         }
         var request = this._portraitRequests[requestId];
         request.callback.fail(null);
         delete this._portraitRequests[requestId];
         return true;
      },

      _onRequestsModified: function() {
         // We're currently processing a request, the inserted request
         // will happen later.
         if (this._currentlyProcessingRequest) {
            return;
         }
         for (var i = this._lastProcessedRequestId; i < this._nextRequestId; ++i) {
            if (i in this._portraitRequests) {
               this._lastProcessedRequestId = i;
               this._processOneRequest(this._portraitRequests[i]);
               return;
            }
         }
      },

      _processOneRequest: function(request) {
         var self = this;
         self._currentlyProcessingRequest = true;
         radiant.call_obj('stonehearth.portrait_renderer', 'render_scene_command', request.scene)
            .done(function(response) {
               delete self._portraitRequests[request.__id];
               self._currentlyProcessingRequest = false;
               request.callback.success(response);
               self._onRequestsModified();
            })
            .fail(function(e) {
               request.callback.fail(e);
            });
      },
   });
})();
