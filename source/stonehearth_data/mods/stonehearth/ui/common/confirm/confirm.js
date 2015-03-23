
// This is the LEGACY confirm view, used by most of the application at this point.
// XXX, remove this once the last call to App.gameView.addView(App.StonehearthConfirmView is gone
App.StonehearthConfirmView = App.View.extend({
   templateName: 'confirmView',
   classNames: ['flex', 'fullScreen'],
   modal: true,

   didInsertElement: function() {
      var self = this;

      this._super();
      var buttons = this.get('buttons');
      var buttonContainer = this.$('#buttons');

      $.each(buttons, function(i, button) {
         var element = $('<button>');

         element.html(button.label);

         if (button.id) {
            element.attr('id', button.id);
         }

         if (button.click) {
            element.click(function() {
               button.click(button.args);
               self.destroy();
            })
         } else {
            element.click(function() {
               self.destroy();
            })
         }

         buttonContainer.append(element);
      });
   },

   destroy: function() {
      if (this.onDestroy) {
         this.onDestroy();
      }

      this._super();
   }
});

// This is the new confirm view, that works the way ember expects. This view gets onto the
// screen by being rendered into an outlet (usually the 'modalmodal' outlet)

App.ConfirmView = App.View.extend({
   classNames: ['flex', 'fullScreen'],
   modal: true,

   actions: {
      clickButton: function(buttonData) {
         if (buttonData && buttonData.click) {
            buttonData.click();
         }
         this.destroy();
      }
   }
});