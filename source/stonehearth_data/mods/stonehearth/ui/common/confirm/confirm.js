
// Common functionality between the save and load views
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

         if (button.click) {
            element.click(function() {
               button.click();
               self.destroy();
            })
         } else {
            element.click(function() {
               self.destroy();
            })
         }

         buttonContainer.append(element);
      });
   }
});
