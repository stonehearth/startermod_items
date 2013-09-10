/*Listen for the event that signals that someone wants to be promoted*/
$(document).ready(function(){
   // When we get the pick people event, show the people picker
   $(top).on("pick_person.stonehearth_items", function (_, e) {
      var view = App.gameView.addView(App.StonehearthPeoplePickerView, {
         uri: '/server/objects/stonehearth_census/worker_tracker',
         dataBag: {talisman: e.entity},
         postURL: e.event_data.post_target
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
      var data = {
         targetPerson: person.__self,
         data: this.dataBag
      };
      var self = this;
      $.ajax({
         type: 'post',
         url: this.postURL,
         contentType: 'application/json',
         data: JSON.stringify(data)
      }).done(function(return_data){
         //TODO: animate out
         self.destroy();
      });
   }

});
