App.StonehearthDemandTributeBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
	templateName: 'demandTributeBulletinDialog',

   _demand_tribute: function() {
   	var array = [];
      var demands = this.get('model.data.demands');
      if (demands) {
      	array = radiant.map_to_array(demands.items);
      }
      this.set('demands', array);
   }.observes('model.data.demands'),
   
});
