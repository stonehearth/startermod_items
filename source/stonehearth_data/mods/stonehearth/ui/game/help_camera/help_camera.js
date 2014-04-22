App.StonehearthHelpCameraView = App.View.extend({
   templateName: 'stonehearthHelpCamera',

   init: function() {
      this._super();
      var self = this;

      radiant.call('stonehearth:load_browser_object', 'stonehearth:help_camera')
         .done(function(o) {
            var anyVisible = true;
            self.camera_vars = o.value;
            if (!self.camera_vars) {
               self.camera_vars = {
                  pan: false,
                  orbit: false,
                  zoom: false
               };
            }
            anyVisible = !(self.camera_vars.pan && self.camera_vars.orbit && self.camera_vars.zoom);

            if (!self.camera_vars.pan) {
               $("#panHint").toggle();  
            }
            if (!self.camera_vars.orbit) {
               $("#orbitHint").toggle();
            }
            if (self.zoomVisible(self.camera_vars)) {
               $("#zoomHint").toggle();
            }
            if (anyVisible) {
               radiant.call('stonehearth:get_camera_tracker')
                  .done(function(o) {
                     this.cameraTrace = radiant.trace(o.camera_tracker)
                        .progress(function(cameraUpdate) {
                           self._updateHints(cameraUpdate);
                        })
                        .fail(function(o) {
                            console.log('failed! ', o)
                        });
                  });
            } else {
               self.destroy();
            }
         });

   },

   zoomVisible: function() {
      return !this.camera_vars.zoom && this.camera_vars.orbit && this.camera_vars.pan;
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
         this.camera_vars.pan = true;
      }

      if (cameraUpdate.orbit) {
         $("#orbitHint").fadeOut();
         this.camera_vars.orbit = true;
      }

      if (cameraUpdate.zoom) {
         $("#zoomHint").fadeOut();
         this.camera_vars.zoom = true;
      }

      if (this.zoomVisible()) {
         $("#zoomHint").fadeIn();
      }

      radiant.call('stonehearth:save_browser_object', 'stonehearth:help_camera', 
         {
            pan : this.camera_vars.pan, 
            orbit : this.camera_vars.orbit,
            zoom : this.camera_vars.zoom
         });

      if (this.camera_vars.pan && this.camera_vars.orbit && this.camera_vars.zoom) {
         this.destroy();
      }
   }

});