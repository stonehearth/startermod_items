// management of modal views 
$(document).ready(function(){
   App.stonehearth.modalStack = []

   $(document).keyup(function(e) {
      if(e.keyCode == 27) {
         if (App.stonehearth.modalStack.length > 0) {
            var modal = App.stonehearth.modalStack.pop();
            modal.ownerView.destroy();
         } else if (App.stonehearth.startMenu.stonehearthMenu("getMenu")){
            App.stonehearth.startMenu.stonehearthMenu("hideMenu")
         } else {
            App.gameView.addView(App.StonehearthEscMenuView);
         }
      }
   });
});

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
         modalOverlay.ownerView = childView;
         this.pushObject(modalOverlay);

         App.stonehearth.modalStack.push(modalOverlay);

      }

      this._viewLookup[type] = childView;
      this.pushObject(childView);
      return childView;
   },

   getView: function(type) {
      return this._viewLookup[type];
   }

});
