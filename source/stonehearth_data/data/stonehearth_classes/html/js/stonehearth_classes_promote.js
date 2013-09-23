$(document).ready(function(){

   $(top).on("promote_citizen.stonehearth_classes", function (_, e) {
      var view = App.gameView.addView(App.StonehearthClassesPromoteView, {
         promoteUri : e.event_data.post_target,
         promoteParams : {talisman: e.entity},
         promotionClass : 'XXX_TODO_INSERT_CLASS_NAME'
      });
   });
});

// Expects the uri to be an entity with a stonehearth_crafter:workshop
// component
App.StonehearthClassesPromoteView = App.View.extend({
   templateName: 'stonehearthClassesPromote',
   modal: true,

   init: function() {
      this._super();
   },

   destroy: function() {
      var self = this;

      this.set('context.citizenToPromote', null);
      this._super();

      /*
      $('#crafterPromoteScroll').fadeOut(function() {
         //...
      });
      */

   },

   didInsertElement: function() {
      if (this.get('context')) {
         $('#crafterPromoteScroll')
            .hide()
            .fadeIn();
      }
   },

   actions: {
      chooseCitizen: function() {
         console.log('instantiate the picker!');

      var self = this;
         var picker = App.gameView.addView(App.StonehearthPeoplePickerView, {
            uri: '/server/objects/stonehearth_census/worker_tracker',
            title: 'Choose the worker to promote', //xxx localize
            css: {
               top: 350,
               left: 105
            },
            callback: function(person) {
               self.set('context.citizenToPromote', person);

               $('#promoteButton')
                  .show()
                  .pulse()
            }
         });
      },

      promoteCitizen: function() {
         var person = this.get('context.citizenToPromote');

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
   },

   dateString: function() {
      var dateObject = App.gameView.getDate();
      var date;
      if (dateObject) {
         date = dateObject.date;
      } else {
         date = "Ooops, clock's broken."
      }
      return App.gameView.getDate().date;
   }
});