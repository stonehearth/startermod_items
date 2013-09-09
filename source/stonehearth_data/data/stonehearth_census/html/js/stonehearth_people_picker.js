App.StonehearthPeoplePickerView = App.View.extend({
   templateName: 'stonehearthPeoplePicker',

   components: {
      'entities': {
         'unit_info' : {}
      }
   },

   init: function() {
      this._super();     
   },

   didInsertElement: function() {
      this.set('uri', '/server/objects/stonehearth_census/worker_tracker');
      console.log('-- start of context --');
      var c = this.get('context')
      console.log(c);
      console.log('-- end of context --');
   },

});
