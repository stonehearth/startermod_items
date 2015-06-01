App.StonehearthTerrainVisionWidget = App.View.extend({
   templateName: 'stonehearthTerrainVision',
   uriProperty: 'context.data',

   init: function() {
      var self = this;

      self._super();

      radiant.call('stonehearth:get_client_service', 'subterranean_view')
         .done(function(e) {
            self.set('uri', e.result);
         })
         .fail(function(e) {
            console.log('error getting subterranean_view client service')
            console.dir(e)
         })
   },


   didInsertElement: function() {
      this._super();

      var self = this;
      
      this.$('#xrayButton').click(function() {
         App.stonehearthClient.subterraneanSetClip(false);

         var currentMode = self.get('context.data.xray_mode')
         var newMode;

         if (!currentMode) {
            newMode = self._lastMode || 'full';
         } else {
            newMode = null;
         }

         App.stonehearthClient.subterraneanSetXRayMode(newMode);
      });

      this.$('.xrayButton').click(function() {
         App.stonehearthClient.subterraneanSetClip(false);

         var currentMode = self.get('context.data.xray_mode')
         var newMode = $(this).attr('mode');

         if (newMode == currentMode) {
            newMode = null;
         }

         self._lastMode = newMode
         App.stonehearthClient.subterraneanSetXRayMode(newMode);
      });

      this.$('#sliceButton').click(function() {
         App.stonehearthClient.subterraneanSetXRayMode(null);

         var button = $(this);
         var currentlyClipping = button.hasClass('clip');

         // toggle slice modes
         if (currentlyClipping) {
            App.stonehearthClient.subterraneanSetClip(false);
         } else {
            App.stonehearthClient.subterraneanSetClip(true);
         }
      });

      this.$('#clipUp').click(function() {
         App.stonehearthClient.subterraneanSetXRayMode(null);
         App.stonehearthClient.subterraneanMoveUp();
      });

      this.$('#clipDown').click(function() {
         App.stonehearthClient.subterraneanSetXRayMode(null);
         App.stonehearthClient.subterraneanMoveDown();
      });

      // bit of a hack given that we can't do hotkey="\", "[", or "]" in the div
      $(document).keyup(function(e) {
         // '\'
         if (e.keyCode == 220) {
            self.$('#sliceButton').click();
            return;
         }

         // ']'
         if (e.keyCode == 221) {
            self.$('#clipUp').click();
            return;
         }

         // '['
         if (e.keyCode == 219) {
            self.$('#clipDown').click();
            return;
         }
      });

      this.$('[title]').tooltipster();
   }
});
