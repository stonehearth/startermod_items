
App.StonehearthView = Ember.ContainerView.extend({
   init: function() {
      this._super();
      this.add(App.MainbarView);
      this.add(App.UnitFrameView);
      this.add(App.ObjectBrowserView);
      //this.add(App.MainbarView, {url: "http://localhost/api/available_workshops.txt"});
      //this.add(App.CraftingWindowView, { url: "http://localhost/api/objects/1.txt"});
      //App.render("unitFrame");

      App.gameView = this;
   },

   add: function(type, options) {
      var childView = this.createChildView(type, {
         classNames: ['sh-view']
      });

      childView.setProperties(options);
      this.pushObject(childView)
   }

});
