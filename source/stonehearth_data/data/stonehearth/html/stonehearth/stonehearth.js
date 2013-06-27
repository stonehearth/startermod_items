
App.StonehearthView = Ember.ContainerView.extend({
   init: function() {
      this._super();
      var self = this;
      
      // xxx: fetch this from the game...
      var json = {
         views : [
            "MainbarView",
            "UnitFrameView",
            "ObjectBrowserView"
         ]
      };

      var views = json.views || [];
      $.each(views, function(i, name) {
         var ctor = App[name]
         if (ctor) {
            self.add_view(ctor);
         }
      });
      //this.add(App.MainbarView, {url: "http://localhost/api/available_workshops.txt"});
      //this.add(App.CraftingWindowView, { url: "http://localhost/api/objects/1.txt"});
   },

   add_view: function(type, options) {
      var childView = this.createChildView(type, {
         classNames: ['stonehearth-view']
      });
      childView.setProperties(options);
      this.pushObject(childView)
   }

});
