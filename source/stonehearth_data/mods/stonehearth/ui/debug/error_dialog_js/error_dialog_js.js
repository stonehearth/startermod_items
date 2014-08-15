App.StonehearthJsErrorDialogView = App.View.extend({
   templateName: 'errorDialogJs',

   init: function() {
      this._super();
   },

   didInsertElement: function() {

      $('#errorDialog')
        .draggable()
        .position({
          my: "right top",
          at: "right top",
          of: "body",
        });
    },

    actions:  {
       close: function() {
          this.destroy();
      }
    }
});
