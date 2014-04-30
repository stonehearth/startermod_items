App.StonehearthGameSpeedWidget = App.View.extend({
   templateName: 'stonehearthGameSpeed',

   didInsertElement: function() {
      var self = this;
      this.$('#playPauseButton').click(function() {
         console.log('toggle play/pause!');
      });

      //TODO, add a tooltip
   }
});
