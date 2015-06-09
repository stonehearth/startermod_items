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
         } else if (App.startMenu && App.startMenu.getMenu()) {
            // if there's an open menu, close it
            App.startMenu.hideMenu();
         } else if (App.getGameMode() != 'normal') {
            // switch to normal mode
            App.stonehearthClient.deactivateAllTools();
            App.setGameMode('normal');
         } else {
            if (App.startMenu) { // TODO (yshan): remove this for better handling of camp selection in the future.
               App.gameView.addView(App.StonehearthEscMenuView);
            }
         }
      }
   });
});

App.ContainerView = Ember.ContainerView.extend({

   _viewLookup: {},

   addView: function(type, options) {
      // console.log("adding view " + type);

      var modalOverlay;
      var childView = this.createChildView(type, {
         classNames: ['stonehearth-view']
      });
      childView.setProperties(options);

      if (childView.get('modal')) {
         modalOverlay = this.createChildView(App.StonehearthModalOverlayView, {
            classNames: ['stonehearth-view'],
            modalView: childView,
         });
         childView.modalOverlay = modalOverlay;
      } 

      if (childView.get('modal') || childView.get('closeOnEsc')) {
         App.stonehearth.modalStack.push(childView);
      }

      this._viewLookup[type] = childView;

      if (modalOverlay) {
         this.pushObject(modalOverlay);
      }
      this.pushObject(childView);

      return childView;
   },

   show: function() {
      this.set('isVisible', true);
   },

   hide: function() {
      this.set('isVisible', false);
   },

   hideAllViews: function() {
      var childViews = this.get('childViews');

      $.each(childViews, function(i, childView) {
         childView.hide();
         //childView.set('isVisible', false);
      });
      App.stonehearthClient.hideTip();
   },

   getView: function(type) {
      return this._viewLookup[type];
   }

});
