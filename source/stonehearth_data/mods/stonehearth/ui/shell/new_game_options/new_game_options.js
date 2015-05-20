App.StonehearthNewGameOptions = App.View.extend({
   templateName: 'stonehearthNewGameOptions',
   classNames: ['flex', 'fullScreen'],
   closeOnEsc: true,

   _options: {
      enable_enemies: true,
      starting_pets: {
         puppy: false,
         kitten: false,
         mammoth: false,
         dragon_whelp: false,
      },
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

         // check if the ks_pet_options mod is installed. If it is, read from the checkboxes
         if (self.get('kickstarter_pets')) {
            self._options.starting_pets.puppy = self.$('#starting_puppy').is(':checked')
            self._options.starting_pets.kitten = self.$('#starting_kitten').is(':checked')
            self._options.starting_pets.mammoth = self.$('#starting_mammoth').is(':checked')            
            self._options.starting_pets.dragon_whelp = self.$('#starting_dragon_whelp').is(':checked')            
         }

         // show the embark view
         App.shellView.addView(App.StonehearthEmbarkView, {options: self._options});

         // hide the title screen so it doesn't flicker later.
         App.shellView.getView(App.StonehearthTitleScreenView).$().hide();
         self.destroy();
      });
   },
});
