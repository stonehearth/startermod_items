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
         self.$('#bulletinNotification').fadeOut(bulletinNotificationFadeTime, function() {
            self.destroy();
         });
      }, bulletinNotificationDuration);

      self.$('#popup').click(function() {
         var bulletin = self.get('context');
         App.bulletinBoard.showDialogView(bulletin);
         self.destroy();
      });
   },

   willDestroyElement: function() {
      App.bulletinBoard.onNotificationViewDestroyed();
      this._super();
   }
});
