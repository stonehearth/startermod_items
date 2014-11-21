App.StonehearthQuestBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
	templateName: 'questBulletinDialog',

   _rewards: function() {
      var map = this.get('context.data.rewards');
      var array = radiant.map_to_array(map);
      this.set('context.data.rewardsArray', array);
   }.observes('context.data.rewards'),
   
});
