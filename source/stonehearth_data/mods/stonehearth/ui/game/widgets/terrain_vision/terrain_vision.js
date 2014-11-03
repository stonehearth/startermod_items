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

      this.$('[title]').tooltipster();
   }
});
