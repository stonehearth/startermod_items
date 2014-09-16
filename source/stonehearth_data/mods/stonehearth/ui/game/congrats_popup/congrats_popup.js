
// Common functionality between the save and load views
App.StonehearthCongratsPopup = App.StonehearthConfirmView.extend({
   templateName: 'congratsPopup',
   classNames: ['flex', 'fullScreen'],

   didInsertElement: function() {
      // XXX, doug, replace this with a clapping 'yay!' sound
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:promotion_menu:stamp'} );
      this._super();
   }
});
