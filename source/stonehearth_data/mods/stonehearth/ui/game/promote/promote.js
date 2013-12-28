$(document).ready(function(){

   $(top).on("radiant_promote_to_profession", function (_, e) {
      var view = App.gameView.addView(App.StonehearthClassesPromoteView, { 
         talisman: e.entity,
         promotionClass : e.event_data.promotion_name
      });
   });
});

// Expects the uri to be an entity with a stonehearth:workshop
// component
App.StonehearthClassesPromoteView = App.View.extend({
   templateName: 'stonehearthClassesPromote',
   modal: false,

   init: function() {
      this._super();
      var self = this;
      radiant.call('stonehearth:get_worker_tracker')
         .done(function(response) {
            radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:promotion_menu:scroll_open' );
            self._worker_tracker = response.tracker;
            self._trace = new RadiantTrace();
            self._trace.traceUri(response.tracker, 
               {
                  'entities': {
                     'unit_info' : {}
                  }
               })
               .progress(function(data) {
                  self._workers = data.entities;
               });
         });

      App.gameView.getView(App.StonehearthUnitFrameView).supressSelection(true);
      $(top).on("radiant_selection_changed.promote_view", function (_, data) {
         var foundWorker = false;
         for (var i = 0; i < self._workers.length; i++) {
            var uri = self._workers[i]['__self']
            if (uri == data.selected_entity) {
               self.set('context.citizenToPromote', self._workers[i]);
               foundWorker = true;
               break;
            }
         }

         if (foundWorker) {
            self._gotoApproveStep();
         } else {
            self.destroy();
         }
      });
      $(top).on('keyup keydown', function(e){
         if (e.keyCode == 27) {
            //If escape, close window
            self.destroy();
         }
      });

   },

   destroy: function() {
      $(top).off("radiant_selection_changed.promote_view");
      App.gameView.getView(App.StonehearthUnitFrameView).supressSelection(false);
      this._super();
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
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:promotion_menu:sub_menu' );
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
                              radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:promotion_menu:select' );
                              self.set('context.citizenToPromote', person);

                              self._gotoApproveStep()
                           }
                        });
            });
      },

      approve: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:promotion_menu:stamp' );
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
                     self.destroy();
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
      var stamp = scroll.find('#approveStamp');
      setTimeout(function () {
         scroll.find('#approveStamper').fadeIn();   
      }, 500);
      
      /*
      stamp.on('mouseover', function() {
         // show the stamper
         stamp.off('mouseover');
         scroll.find('#approveStamper').fadeIn();
      })
      */
   },

   _promoteCitizen: function() {
      var self = this;
      var person = this.get('context.citizenToPromote').__self;

      $('#promoteScroll').animate({ 'bottom' : -400 }, 200, function() { 
         radiant.call('stonehearth:grab_promotion_talisman', person, self.talisman)
            .done(function(data) {
               radiant.log.info("promote finished!", data)
            })
            .fail(function(data) {
               radiant.log.warning("promote failed:", data)
            });

         self.destroy();
      });
   }
});