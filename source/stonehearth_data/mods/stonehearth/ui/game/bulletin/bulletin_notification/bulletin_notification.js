App.StonehearthBulletinNotification = App.View.extend({
	templateName: 'bulletinNotification',
   uriProperty: 'model',

   init: function() {
      this._super();
   },
   
   didInsertElement: function() {
      // TODO: read these contsants from config
      var bulletinNotificationDuration = 4000;
      var bulletinNotificationFadeTime = 500;
      var self = this;
      this._super();

      self.$('#bulletinNotification').pulse();

      var bulletin = this.get('model');

      if (!bulletin.sticky) {
         setTimeout(function() {
            var element = self.$('#bulletinNotification');

            // make sure element still exists
            if (element) {
               element.fadeOut(bulletinNotificationFadeTime, function() {
                  self.destroy();
               });
            }
         }, bulletinNotificationDuration);
      }

      self.$('#bulletinNotification').click(function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:page_down'});
         var bulletin = self.get('model');
         App.bulletinBoard.zoomToLocation(bulletin);
         App.bulletinBoard.showDialogView(bulletin);
         // Note: don't need to call self.destroy() because showDialogView will try to do that for us.
      });

      if (this.get('model.type') == 'alert') {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:scenarios:alert'} );
      } else {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:scenarios:caravan'} );   
      }
      
   },

   willDestroyElement: function() {
      var self = this;
      var bulletin = self.get('model');
      App.bulletinBoard.onNotificationViewDestroyed(bulletin);
      this._super();
   }
});
