App.StonehearthRedAlertWidget = App.View.extend({
   templateName: 'stonehearthRedAlert',

   didInsertElement: function() {
      this._super();

      this.$('#redAlert').click(function() {
        App.stonehearthClient.rallyWorkers();
      });

      this.$('#redAlert').pulse();

   }
});
