App.StonehearthHelpCameraView = App.View.extend({
   templateName: 'stonehearthHelpCamera',

   init: function() {
      this._super();
      var self = this;

      radiant.call('stonehearth:get_camera_tracker')
         .done(function(o) {
            this.cameraTrace = radiant.trace(o.camera_tracker)
               .progress(function(cameraUpdate) {
                  self._updateHints(cameraUpdate);
               })
               .fail(function(o) {
                   console.log('failed! ', o)
               })
         });
   },

   destroy: function() {
      if (this.cameraTrace) {
         this.cameraTrace.destroy();
      }
      this._super();
   },

   didInsertElement: function() {

   },

   _updateHints: function(cameraUpdate) {
      if (cameraUpdate.pan) {
         $("#panHint").fadeOut();
         this.pan = true;
      }

      if (cameraUpdate.orbit) {
         $("#orbitHint").fadeOut();
         this.orbit = true;
      }

      if (cameraUpdate.zoom) {
         $("#zoomHint").fadeOut();
         this.zoom = true;
      }

      if (this.pan && this.orbit) {
         $("#zoomHint").fadeIn();
      }

      if (this.pan && this.orbit && this.zoom) {
         this.destroy();
      }
   }

});