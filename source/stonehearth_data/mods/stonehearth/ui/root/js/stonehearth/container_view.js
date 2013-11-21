
App.ContainerView = Ember.ContainerView.extend({

   _viewLookup: {},

   addView: function(type, options) {
      console.log("adding view " + type);

      var childView = this.createChildView(type, {
         classNames: ['stonehearth-view']
      });
      childView.setProperties(options);

      if (childView.get('modal')) {
         var modalOverlay = this.addView('StonehearthModalOverlay', { modalView: childView });
         childView.modalOverlay = modalOverlay;
         this.pushObject(modalOverlay);
      }

      this._viewLookup[type] = childView;
      this.pushObject(childView);
      return childView;
   },

   getView: function(type) {
      return this._viewLookup[type];
   }

});
