
$(document).ready(function(){
  
  App.stonehearth.bulletin = {
    lastBulletinTimestamp: -1,
    notificationView: null
  }

  $(top).on("new_bulletin.bulletin_notification", function (_, bulletinData) {
    var bulletin = bulletinData.bulletins[bulletinData.bulletins.length - 1];
    
    if (App.stonehearth.bulletin.view) {
       App.stonehearth.bulletin.view.destroy();
    }
    
    App.stonehearth.bulletin.view = App.gameView.addView(App.StonehearthBulletinNotification, { context: bulletin });
  });  
});

App.StonehearthBulletinNotification = App.View.extend({
	templateName: 'bulletinNotification',

   init: function() {
      this._super();
      var self = this;
   },

   didInsertElement: function() {
      this._super();
      this.$('#bulletinNotification').pulse();
   }  
});
