App.StonehearthJsErrorDialogView = App.View.extend({
   templateName: 'errorDialogJs',

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      $('#errorDialogjs')
        .draggable()
          .position({
            my: "right+30 top+100",
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
