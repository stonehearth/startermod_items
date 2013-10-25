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
                              left: 703,
                              bottom: 50
                           },
                           callback: function(person) {
                              self.set('context.citizenToPromote', person);

                              self._gotoApproveStep()
                           }
                        });
            });
      },

      approve: function() {
         var self = this;
         // animate down
         $('#approveStamper').animate({ bottom: 60 }, 100 , function() {
            //animate up
            $('#approvedStamp').show();
            $(this)
               .delay(200)
               .animate({ bottom: 200 }, 150, function () {
                  //promote the citizen after a short delay
                  setTimeout(function() {
                     self._promoteCitizen();
                  }, 1000);
               });
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
   },

   _gotoApproveStep: function() {
      var scroll = $('#promoteScroll');
      // scroll the scroll up
      scroll.animate({ 'bottom' : -50 }, 200, function() { 
         // on mouseover of the stamp
         var stamp = scroll.find('#approveStamp');
         stamp.on('mouseover', function() {
            // show the stamper
            stamp.off('mouseover');
            scroll.find('#approveStamper').fadeIn();
         })
      }); 
   },

   _promoteCitizen: function() {
      var self = this;
      var person = this.get('context.citizenToPromote').__self;

      $('#promoteScroll').animate({ 'bottom' : -400 }, 200, function() { 
         radiant.call('stonehearth:grab_promotion_talisman', person, self.talisman)
            .done(function(data) {
               radiant.log.info("promote_citizen.stonehearth finished!", data)
            })
            .fail(function(data) {
               radiant.log.warning("promote_citizen.stonehearth failed:", data)
            });

         self.destroy();
      });
   }
});