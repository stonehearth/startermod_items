App.StonehearthQuestBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
	templateName: 'questBulletinDialog',

   _rewards: function() {
      this._mapToArray('context.data.rewards', 'context.data.rewardsArray');
   }.observes('context.data.rewards'),
   
});
