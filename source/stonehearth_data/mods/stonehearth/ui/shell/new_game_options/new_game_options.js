App.StonehearthNewGameOptions = App.View.extend({
   templateName: 'stonehearthNewGameOptions',
   classNames: ['flex', 'fullScreen'],
   closeOnEsc: true,

   didInsertElement: function() {
      this._super();
      var self = this;

      this.$('.newGameButton').click(function() {
         var enemies = true
         if ($(this).attr('id') == 'enemiesOn') {
            enemies = true
         } else {
            enemies = false
         }

         radiant.call('stonehearth:set_game_options', {
               enable_enemies: enemies
            }).done(function() {
               // show the embark view
               App.shellView.addView(App.StonehearthEmbarkView);

               // hide the title screen so it doesn't flicker later.
               App.shellView.getView(App.StonehearthTitleScreenView).$().hide();
               self.destroy();
            });
      });
   }

});
