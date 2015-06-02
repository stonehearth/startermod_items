App.StonehearthNewGameOptions = App.View.extend({
   templateName: 'stonehearthNewGameOptions',
   classNames: ['flex', 'fullScreen'],
   closeOnEsc: true,

   _options: {
      enable_enemies: true,
      starting_talismans : [
         "stonehearth:carpenter:talisman", // Always have at least the carpenter.
      ],
      starting_pets: [
      ],
      starting_gold: 0,
   },

   _doInit: function() {
      var modules = App.getModuleData();

      if (modules.kickstarter_pets) {
         this.set('kickstarter_pets', true);
      }
   }.on('init'),

   didInsertElement: function() {
      this._super();
      var self = this;

      // trigger that the view is up, so that mods can add their own new game options
      $(document).trigger('stonehearthNewGameOptions', this);

      this.$('.newGameButton').click(function() {
         if ($(this).attr('id') != 'enemiesOn') {
            self._options.enable_enemies = false
         }
         
         // show the embark view
         App.shellView.addView(App.StonehearthEmbarkView, {_options: self._options});

         // hide the title screen so it doesn't flicker later.
         App.shellView.getView(App.StonehearthTitleScreenView).$().hide();
         self.destroy();
      });
   },
});
