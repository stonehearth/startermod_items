App.StonehearthHelpCameraView = App.View.extend({
   templateName: 'stonehearthHelpCamera',

   init: function() {
      this._super();
      var self = this;

      radiant.call('radiant:get_config', 'hide_help.camera')
         .done(function(o) {
            self._cameraVars = o;

            if (!self._cameraVars.pan) {
               $("#panHint").show();
            }
            if (!self._cameraVars.orbit) {
               $("#orbitHint").show();
            }
            if (self.showZoom()) {
               $("#zoomHint").show();
            }

            var anyVisible = !(self._cameraVars.pan && self._cameraVars.orbit && self._cameraVars.zoom);
            if (anyVisible) {
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
            }
         });

      /*
      radiant.call('stonehearth:load_browser_object', 'stonehearth:help_camera')
         .done(function(o) {
            var anyVisible = true;
            self._cameraVars = o.value;
            if (!self._cameraVars) {
               self._cameraVars = {
                  pan: false,
                  orbit: false,
                  zoom: false
               };
            }
            anyVisible = !(self._cameraVars.pan && self._cameraVars.orbit && self._cameraVars.zoom);

            if (!self._cameraVars.pan) {
               $("#panHint").toggle();  
            }
            if (!self._cameraVars.orbit) {
               $("#orbitHint").toggle();
            }
            if (self.showZoom(self._cameraVars)) {
               $("#zoomHint").toggle();
            }
            if (anyVisible) {
            } else {
               self.destroy();
            }
         });
      */
   },

   showZoom: function() {
      return !this._cameraVars.zoom && this._cameraVars.orbit && this._cameraVars.pan;
   },

   destroy: function() {
      if (this.cameraTrace) {
         this.cameraTrace.destroy();
      }
      this._super();
   },

   _updateHints: function(cameraUpdate) {
      if (cameraUpdate.pan) {
         $("#panHint").fadeOut();
         this._cameraVars.pan = true;
      }

      if (cameraUpdate.orbit) {
         $("#orbitHint").fadeOut();
         this._cameraVars.orbit = true;
      }

      if (cameraUpdate.zoom) {
         $("#zoomHint").fadeOut();
         this._cameraVars.zoom = true;
      }

      if (this.showZoom()) {
         $("#zoomHint").fadeIn();
      }

      radiant.call('radiant:set_config', 'hide_help.camera', 
         {
            pan : this._cameraVars.pan, 
            orbit : this._cameraVars.orbit,
            zoom : this._cameraVars.zoom
         });

      /*
      radiant.call('stonehearth:save_browser_object', 'stonehearth:help_camera', 
         {
            pan : this._cameraVars.pan, 
            orbit : this._cameraVars.orbit,
            zoom : this._cameraVars.zoom
         });
      */

      if (this._cameraVars.pan && this._cameraVars.orbit && this._cameraVars.zoom) {
         this.destroy();
      }
   }

});