App.StonehearthCollectionQuestBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
	templateName: 'collectionQuestBulletinDialog',


   didInsertElement: function() {
      this._super();
      this._wireButtonToCallback('#collectionPayButton',    'collection_pay_callback');
      this._wireButtonToCallback('#collectionCancelButton', 'collection_cancel_callback');
   },

   _demands: function() {
   	var array = [];
      var demands = this.get('model.data.demands');
      if (demands) {
      	array = radiant.map_to_array(demands.items);
      }
      this.set('demands', array);
   }.observes('model.data.demands'),
   
});
