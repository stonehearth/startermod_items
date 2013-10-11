
App.ContainerView = Ember.ContainerView.extend({

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

      this.pushObject(childView);
      return childView;
   },

});
