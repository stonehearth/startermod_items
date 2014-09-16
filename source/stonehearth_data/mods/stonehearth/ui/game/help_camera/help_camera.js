App.StonehearthHelpCameraView = App.View.extend({
   templateName: 'stonehearthHelpCamera',

   init: function() {
      this._super();
      var self = this;

      $("#panHint").show();
      $("#orbitHint").show();

      radiant.call('stonehearth:get_camera_tracker')
         .done(function(o) {
            self.cameraTrace = radiant.trace(o.camera_tracker)
               .progress(function(cameraUpdate) {
                  self._updateHints(cameraUpdate);
               })
               .fail(function(o) {
                   console.log('failed! ', o)
               });
         });
   },

   showZoom: function() {
      return !this._zoomDone && this._orbitDone && this._panDone;
   },

   destroy: function() {
      if (this.cameraTrace) {
         this.cameraTrace.destroy();
      }
      this._super();
   },

   _updateHints: function(cameraUpdate) {
      var self = this;

      if (cameraUpdate.pan) {
         $("#panHint").fadeOut();
         this._panDone = true;
      }

      if (cameraUpdate.orbit) {
         $("#orbitHint").fadeOut();
         this._orbitDone = true;
      }

      if (cameraUpdate.zoom) {
         $("#zoomHint").fadeOut();
         this._zoomDone = true;
      }

      if (this.showZoom()) {
         $("#zoomHint").fadeIn();

         // hide the zoom hint after a few seconds if it's not already gone
         setTimeout(function() {
            if (!self._zoomDone) {
               $("#zoomHint").fadeOut(250, function() {
                  self._createCamp();
               });
               
            }
         }, 3000);
      }

      var allDone = this._zoomDone && this._panDone && this._orbitDone;
      if (allDone) {
         this._createCamp();
      }
   },

   _createCamp: function() {
      App.gameView.addView(App.StonehearthCreateCampView)
      this.destroy();      
   }

});