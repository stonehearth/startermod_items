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
          
      var versionView = App.gameView.getView(App.StonehearthVersionView);
      var version = versionView.$('#versionTag')[0];
      this.set('context.game_version', version.innerText);

      this.$('#copyButton').click(function() {
         var range = document.createRange();
         range.selectNodeContents($('#errorDialog')[0]);
         window.getSelection().addRange(range);
         document.execCommand('copy');
         window.getSelection().removeAllRanges();
      });
    },

    actions:  {
       close: function() {
          this.destroy();
      }
    }
});
