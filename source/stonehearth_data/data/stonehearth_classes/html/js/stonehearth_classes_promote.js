$(document).ready(function(){

   $(top).on("promote_citizen.stonehearth_classes", function (_, e) {
      var promoteUri = e.event_data.post_target;
      var promoteParams =  {talisman: e.entity};

      var view = App.gameView.addView(App.StonehearthClassesPromoteView, { 
         promoteUri : promoteUri,
         promoteParams : promoteParams
      });
   });
});

// Expects the uri to be an entity with a stonehearth_crafter:workshop
// component
App.StonehearthClassesPromoteView = App.View.extend({
   templateName: 'stonehearthClassesPromote',

   init: function() {
      this._super();
   },

   destroy: function() {
      radiant.keyboard.setFocus(null);
      this._super();
   },

   actions: {

      chooseCitizen: function() {
         console.log('instantiate the picker!');

         var self = this;
         $(top).trigger("pick_person.stonehearth_census", {
            uri: '/server/objects/stonehearth_census/worker_tracker',
            callback: function(person) {
               self.promoteCitizen(person);
            }
         });
      },
   },

   promoteCitizen: function(person) {

      var data = {
         targetPerson: person.__self,
         data: this.promoteParams
      };

      var self = this;
      $.ajax({
         type: 'post',
         url: this.promoteUri,
         contentType: 'application/json',
         data: JSON.stringify(data)
      }).done(function(return_data){
         self.destroy();
      });

   }

});