App.StonehearthBulletinNotification = App.View.extend({
	templateName: 'bulletinNotification',

   didInsertElement: function() {
      // TODO: read these contsants from config
      var bulletinNotificationDuration = 4000;
      var bulletinNotificationFadeTime = 500;
      var self = this;
      this._super();

      self.$('#bulletinNotification').pulse();

      setTimeout(function() {
         var element = self.$('#bulletinNotification');

         // make sure element still exists
         if (element) {
            element.fadeOut(bulletinNotificationFadeTime, function() {
               self.destroy();
            });
         }
      }, bulletinNotificationDuration);

      self.$('#popup').click(function() {
         var bulletin = self.get('context');
         App.bulletinBoard.zoomToLocation(bulletin);
         App.bulletinBoard.showDialogView(bulletin);
         self.destroy();
      });

      if (this.get('context.type') == 'alert') {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:scenarios:alert' );
      } else {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:scenarios:caravan' );   
      }
      
   },

   willDestroyElement: function() {
      var self = this;
      var bulletin = self.get('context');
      App.bulletinBoard.onNotificationViewDestroyed(bulletin);
      this._super();
   }
});
