App.StonehearthTerrainVisionWidget = App.View.extend({
   templateName: 'stoneheartTerrainVision',
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
      
      // hide menus that are in development
      radiant.call('radiant:get_config', 'show_in_progress_ui')
         .done(function(response) {
            if (!response.show_in_progress_ui) {
               self.destroy();
               return;
            }
         })      

      this.$('#visionButton').click(function() {
         var button = $(this);
         var currentlyClipping = button.hasClass('clip');

         // toggle vision modes
         if (currentlyClipping) {
            App.stonehearthClient.subterraneanSetClip(false);
            button.removeClass('clip');
            self.$('#palette').hide();
         } else {
            App.stonehearthClient.subterraneanSetClip(true);
            button.addClass('clip');
            self.$('#palette').show();
         }
      });

      this.$('#clipUp').click(function() {
         App.stonehearthClient.subterraneanMoveUp();
      });

      this.$('#clipDown').click(function() {
         App.stonehearthClient.subterraneanMoveDown();
      });

      // bit of a hack given that we can't do hotkey="\" in the div
      $(document).keyup(function(e) {
         if(e.keyCode == 220) {
            self.$('#visionButton').click();
         }
      });

      this.$('[title]').tooltipster();
   }
});
