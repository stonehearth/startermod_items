App.StonehearthQuestBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
	templateName: 'questBulletinDialog',

   _rewards: function() {
      var rewards = this.get('context.data.rewards');      
      this.set('context.data.rewardsArray', radiant.map_to_array('rewards'));
   }.observes('context.data.rewards'),
   
});
