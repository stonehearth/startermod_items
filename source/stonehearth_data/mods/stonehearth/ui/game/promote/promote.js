$(document).ready(function(){

   $(top).on("promote_citizen.stonehearth", function (_, e) {
      var view = App.gameView.addView(App.StonehearthClassesPromoteView, { 
         talisman: e.entity,
         promotionClass : 'XXX_TODO_INSERT_CLASS_NAME'
      });
   });
});

// Expects the uri to be an entity with a stonehearth:workshop
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
         radiant.call('stonehearth:get_worker_tracker')
            .done(function(response) {
               App.gameView.addView(App.StonehearthPeoplePickerView, {
                           uri: response.tracker,
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
            });
      },

      promoteCitizen: function() {
         var self = this;
         var person = this.get('context.citizenToPromote').__self;
         radiant.call('stonehearth:grab_promotion_talisman', person, this.talisman)
            .done(function(data) {
               radiant.log.info("promote_citizen.stonehearth finished!", data)
            })
            .fail(function(data) {
               radiant.log.warning("promote_citizen.stonehearth failed:", data)
            })
            .always(function(return_data){
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
      return date;
   }
});