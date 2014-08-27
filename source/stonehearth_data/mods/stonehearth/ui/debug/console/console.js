$(document).ready(function(){
      $(top).bind('keyup', function(e){
         if (e.keyCode == 192)  { // tilde
            var view = App.debugView.getView(App.StonehearthConsoleView);

            if (view && view.$()) {
               view.$().toggle();
               if (view.$().is(':visible')) {
                  view.focus();   
               }
            }
         }
      });
});

App.StonehearthConsoleView = App.View.extend({
   templateName: 'console',

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      var self = this;

      this.$('#input').keypress(function(e) {        
         if (e.which == 13) { // return
            var command = $(this).val();
            $(this).val('');

            radiant.console.run(command);
            //xxx, call something on the server. In the done handler, call _appendToConsole?
            //self._appendToConsole(command)            
         }
      });

      this.$().hide();
      radiant.console.setContainer(self.$('.output'));
   },

   focus: function() {
      this.$('#input')
         .val('')
         .focus();
   },

   _appendToConsole: function(message) {
      var self = this;
      
      self.$('.output').append('\n' + message);
   }

});
