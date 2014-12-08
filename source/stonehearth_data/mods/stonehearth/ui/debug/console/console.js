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
      this._history = [];
      this._curHistoryIdx = 0;
      this._currentCommand = '';
   },

   didInsertElement: function() {
      var self = this;

      this.$('#input').keypress(function(e) {        
         if (e.which == 13) { // return
            var command = $(this).val();
            self._history.push(command);
            $(this).val('');
            self._curHistoryIdx = self._history.length - 1;
            radiant.console.run(command);
         }
      });

      this.$('#input').keyup(function(e) {        
         if (e.which == 38) {  // Up
            if (self._curHistoryIdx == self._history.length - 1) {
               // Remember our current command, if any.
               if ($(this).val() != '') {
                  self._currentCommand = $(this).val();
               }
            }

            $(this).val(self._history[self._curHistoryIdx]);

            self._curHistoryIdx--;
            if (self._curHistoryIdx < 0) {
               self._curHistoryIdx = 0;
            }
         } else if (e.which == 40) { // Down
            if (self._curHistoryIdx == self._history.length - 1) {
               if (self._currentCommand != '') {
                  $(this).val(self._currentCommand);
                  self._currentCommand = '';
               }
               return;
            }
            self._curHistoryIdx++;
            $(this).val(self._history[self._curHistoryIdx]);
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
});
