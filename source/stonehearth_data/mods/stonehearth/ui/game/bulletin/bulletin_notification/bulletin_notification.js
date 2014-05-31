
$(document).ready(function(){
  
   App.stonehearth.bulletinBoard = {
      notificationView: null
   }

   $(top).on("bulletin_board_changed.alert_widget", function (_, bulletinuri) {
         if (App.stonehearth.bulletinBoard.view) {
           App.stonehearth.bulletinBoard.view.destroy();
         }

         App.stonehearth.bulletinBoard.view = App.gameView.addView(App.StonehearthBulletinNotification, { uri: bulletinuri });
      });
});

App.StonehearthBulletinNotification = App.View.extend({
	templateName: 'bulletinNotification',

   didInsertElement: function() {
      this._super();
      this.$('#bulletinNotification').pulse();
   }
});
