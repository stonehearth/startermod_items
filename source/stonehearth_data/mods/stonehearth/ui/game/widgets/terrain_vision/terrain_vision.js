App.StonehearthTerrainVisionWidget = App.View.extend({
   templateName: 'stoneheartTerrainVision',

   didInsertElement: function() {
      this._super();


      this.$('#visionButton').click(function() {
         var button = $(this);
         var currentlyClipping = button.hasClass('clip');

         // toggle vision modes
         if (currentlyClipping) {
            App.stonehearthClient.subterraneanSetClip(false);
            button.removeClass('clip');
         } else {
            App.stonehearthClient.subterraneanSetClip(true);
            button.addClass('clip');
         }
      });

      this.$('#visionButton').tooltipster();
   }
});
