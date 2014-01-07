App.StonehearthModalOverlayView = App.View.extend({
   templateName: 'stonehearthModalOverlay',

   init: function() {
      this._super();
   },

   destroy: function() {
      var self = this;
      /*
      $("#modalOverlay")
         .animate({ opacity: 0.5 }, {
            duration: 300, 
            easing: 'easeInQuad',
            complete: function() {
               radiant.keyboard.setFocus(null);
               if (self.modalView) {
                  self.modalView.destroy();
               }
               self._super();               
            }
         });
      */
      radiant.keyboard.setFocus(null);
      this._super();
   },

   didInsertElement: function() {
      radiant.keyboard.setFocus(this.modalView);
   },

   actions: {
      dismiss: function () {
         if (this.modalView) {
            if (this.modalView.dismiss) {
               this.modalView.dismiss();
            }
            this.modalView.destroy();
         }
         this.destroy();
      }
   }
});