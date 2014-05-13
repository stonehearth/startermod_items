// The view that shows a list of citizens and lets you promote one
App.StonehearthTownView = App.View.extend({
	templateName: 'town',
   classNames: ['flex', 'fullScreen'],

   init: function() {
      var self = this;
      this._super();
   },

   didInsertElement: function() {
      var self = this;
      this._super();

      this._progressbar = this.$("#netWorthBar")
      this._progressbar.progressbar({
         value: 55
      });

      this.$('.scoreBar').progressbar({
         value: 50
      });
   },

});

