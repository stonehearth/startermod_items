App.StonehearthNewGameOptions = App.View.extend({
   templateName: 'stonehearthNewGameOptions',
   classNames: ['flex', 'fullScreen'],
   closeOnEsc: true,

   didInsertElement: function() {
      this._super();
      var self = this;

      this.$('.newGameButton').click(function() {
         var options = {
            enable_enemies: true
         };
         if ($(this).attr('id') != 'enemiesOn') {
            options.enable_enemies = false
         }

         // show the embark view
         App.shellView.addView(App.StonehearthEmbarkView, {options: options});

         // hide the title screen so it doesn't flicker later.
         App.shellView.getView(App.StonehearthTitleScreenView).$().hide();
         self.destroy();
      });
   }

});
