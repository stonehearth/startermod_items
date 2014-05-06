$(document).ready(function(){

   $(top).on("radiant_promote_to_profession", function (_, e) {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:promotion_menu:scroll_open' );
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
   components: {
      'citizens' : {
         '*' : {
            'stonehearth:profession' : {},
            'unit_info': {}
         }
      }
   },

   _worker_filter_fn: function(person) {
      return person['stonehearth:profession'].profession_id == 'worker';
   },

   _buildPeopleArray: function() {
      var vals = [];
      var citizenMap = this.get('context.citizens');
      var self = this;
     
      if (citizenMap) {
         $.each(citizenMap, function(k ,v) {
            if(k == '__self' || !citizenMap.hasOwnProperty(k)) {
               return;
            }

            if (self._worker_filter_fn(v)) {
               vals.push(v);
            }
         });
      }

      this.set('context.citizensArray', vals);
   }.observes('context.citizens.[]'),

   init: function() {
      this._super();
      var self = this;

      App.gameView.getView(App.StonehearthUnitFrameView).supressSelection(true);
      
      radiant.call('stonehearth:get_population')
         .done(function(response) {
            self.set('uri', response.population);
         })

      $(top).on("radiant_selection_changed.promote_view", function (_, data) {
         var workers = self.get('context.citizensArray');
         if (!workers) {
            return;
         }

         var foundWorker = false;
         for (var i = 0; i < workers.length; i++) {
            var uri = workers[i]['__self']
            if (uri == data.selected_entity) {
               self.set('context.selectedUnitName', workers[i].unit_info.name);
               self._selectedUnitUri = uri;
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
      if (this.get('context') != null) {
         self._selectedUnitUri = undefined;
         this.set('context.selectedUnitName', undefined);
      }

      if (this._trace) {
         this._trace.destroy();
      }

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
         App.gameView.addView(App.StonehearthPeoplePickerView, {
                     uri: self.get('uri'),
                     title: 'Choose the worker to promote', //xxx localize
                     css: {
                        left: 703,
                        bottom: 50
                     },
                     // A user-specified function to filter the citizens that come back from
                     // the server.
                     filter_fn: self._worker_filter_fn,
                     // Additional components of the citizen entity that we want retrieved
                     // (for our function).
                     additional_components: { "stonehearth:profession" : {} },
                     callback: function(person) {
                        // `person` is part of the people picker's model.  it will be destroyed as
                        // soon as that view is destroyed, which will happen immediately after the
                        // callback is fired!  scrape everything we need out of it before this happens.
                        radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:promotion_menu:select' );
                        self.set('context.selectedUnitName', person.unit_info.name);
                        self._selectedUnitUri = person.__self;
                        self._gotoApproveStep()
                     }
                  });
      },

      approve: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:promotion_menu:stamp' );
         var self = this;

         App.gameView.getView(App.StonehearthUnitFrameView).supressSelection(false);
         
         self._promoteCitizen();

         // animate down
         $('#approveStamper').animate({ bottom: 60 }, 100 , function() {
            //animate up
            $('#approvedStamp').show();
            $(this)
               .delay(200)
               .animate({ bottom: 200 }, 150, function () {
                  //promote the citizen after a short delay
                  setTimeout(function() {
                     $('#promoteScroll').animate({ 'bottom' : -400 }, 200, function() { 
                        self.destroy();
                     });
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

      if (!self._selectedUnitUri) {
         self.destroy();
         return;
      }
      
      radiant.call('stonehearth:grab_promotion_talisman', self._selectedUnitUri, self.talisman)
         .done(function(data) {
            radiant.log.info("promote finished!", data)
         })
         .fail(function(data) {
            radiant.log.warning("promote failed:", data)
         });

   }
});