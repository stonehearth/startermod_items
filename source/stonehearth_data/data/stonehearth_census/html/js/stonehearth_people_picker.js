/*Listen for the event that signals that someone wants to be promoted*/
$(document).ready(function(){
   // When we get the pick people event, show the people picker
   $(top).on("pick_person.stonehearth_census", function (_, e) {
      var view = App.gameView.addView(App.StonehearthPeoplePickerView, {
         uri: e.uri,
         callback: e.callback
      });
   });
});

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
      //TODO: animate in
   },

   selectPerson: function(person) {
      this.callback(person);
      this.destroy();
   }

});
