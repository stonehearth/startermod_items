// management of modal views 
$(document).ready(function(){
   App.stonehearth.modalStack = []

   $(document).keyup(function(e) {
      if(e.keyCode == 27) {
         if (App.stonehearth.modalStack.length > 0) {
            // there's a modal. close it
            var modal = App.stonehearth.modalStack.pop();

            // it's possible that the view was destroyed by someone else. If so, just pop it 
            while(modal.isDestroyed && App.stonehearth.modalStack.length > 0) {
               modal = App.stonehearth.modalStack.pop();               
            }

            modal.destroy();
         } else if (App.getGameMode() != 'normal') {
            // switch to normal mode
            App.setGameMode('normal');
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
      } 

      if (childView.get('modal') || childView.get('closeOnEsc')) {
         App.stonehearth.modalStack.push(childView);
      }

      this._viewLookup[type] = childView;
      this.pushObject(childView);
      return childView;
   },

   getView: function(type) {
      return this._viewLookup[type];
   }

});
