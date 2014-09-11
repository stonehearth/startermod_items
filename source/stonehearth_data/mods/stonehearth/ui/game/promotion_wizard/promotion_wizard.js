$(document).ready(function(){
   $(top).on("radiant_promote_to_profession", function (_, e) {
      var r  = new RadiantTrace()
      var components = { 
            'unit_info' : {},
            'stonehearth:promotion_talisman' : {} 
         };

      // grab the properties from the talisman and pass them along to the promotion wizard
      r.traceUri(e.entity, components)
         .progress(function(eobj) {
               // eobj could be either a talisman or the person to promote              
               var talisman = eobj['stonehearth:promotion_talisman'] ? eobj : null;
               var citizen = !eobj['stonehearth:promotion_talisman'] ? eobj : null;

               App.gameView.addView(App.StonehearthPromotionWizard, { 
                  talisman: talisman,
                  citizen: citizen
               });
               r.destroy();
            });
   });
});

App.StonehearthPromotionWizard = App.View.extend({
	templateName: 'promotionWizard',

   components: {
      "jobs" : {
      }
   },

   // Making this override 'init' will cause the 'set' method to misbehave, and the trace will not be 
   // observed (from what I can gather, this is because the view is still being constructed and Ember's 
   // manual proto records aren't ready.  Look at 'propertyDidChange' in ember.js, which returns
   // if proto == obj, which will be true during 'init', but not after.)
   //
   // I think Ember might be too complicated.
   onInit: function() {
      this.set('uri', 'stonehearth:professions:index');

      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:page_up'} );
   }.on('init'),

   destroy: function() {
      $(top).off("radiant_selection_changed.promote_view");
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:page_down'} );
      this._super();
   },

   // Called once, when our uri trace first fires, and AFTER our DOM is ready.
   onRenderedUri: function() {
      this.initWizardState();
   },

   didInsertElement: function() {
      var self = this;
      this._super();

      // reset the help text when hovering outside of the job selection panel
      this.$('#jobs').hover( 
         function() {

         },
         function() {
            self.$('#info #name').html(i18n.t('stonehearth:citizens_choose_job_subtitle'));
            self.$('#info #description').html(i18n.t('stonehearth:citizens_choose_job_description'));
         });

      // set the help text to the job description when hovering over each job button
      this.$('.jobButton').hover(
         function() {
            var professionInfo = self.getProfessionInfo($(this).attr('id'));
            var requirements = $('<p>');

            requirements.html('Requires: ' + professionInfo.talisman_uri + '</p>');

            if ($(this).hasClass('locked')) {
               requirements.addClass('locked');
            }

            var description = requirements.html() + professionInfo.description;

            self.$('#info #name').html(professionInfo.name);
            self.$('#info #description').html(description);

         },
         function() {
            self.$('#info #name').html('');
            self.$('#info #description').html('');
         });

      this.$('.jobButton').click(function() {
         if(! $(this).hasClass('locked')) {
            var professionInfo = self.getProfessionInfo($(this).attr('id'));
            self.set('profession', professionInfo);
            self.set('talismanUri', $(this).attr('talisman_uri'));
            self.$('#finishPage').show();
         }
      })

      this.$('#closeButton').click(function() {
         self.invokeDestroy();
      });

      this.$('#finishPage #backButton').click(function() {
         if (self.$('#jobsPage').is(':visible')) {
            self.$('#finishPage').hide(); 
         } else {
            self.invokeDestroy();
         }
        
      })

      this.$('#chooseButton').click(function() {
         self.chooseCitizen();
      });

      this.$('#approveStamper').click(function() {
         self.approve();
      });

      $(top).on("radiant_selection_changed.promote_view", function (_, data) {
         var selected = data.selected_entity;
         App.population.getTrace().done(function(pop) {
            $.each(pop.citizens, function(k, citizen) {
               var uri = citizen['__self']
               if (uri && uri == data.selected_entity) {
                  var profession = citizen['stonehearth:profession']['profession_uri']['alias'];

                  if (profession = 'stonehearth:professions:worker') {
                     self.set('citizen', citizen);
                     self.showApproveStamper();                  
                  }
               }
            });            
         });
      });
   },

   initWizardState: function() {
      var self = this;
      
      // if the talisman is specified
      var talisman = this.get('talisman');
      if (talisman) {
         this.set('profession', this.getProfessionInfo(talisman['stonehearth:promotion_talisman'].profession) );
         this.$('#jobsPage').hide();
         this.$('#finishPage').show();
         this.$('#finishPage #backButton').html( i18n.t('stonehearth:cancel'));
      } else {
         radiant.call('stonehearth:get_talismans_in_explored_region')
            .done(function(e) {
               self.unlockJobs(e.available_professions);
            });         
      }

      // if the citizen is specified
      if (this.get('citizen')) {
         this.showApproveStamper();
      }
   },

   chooseCitizen: function() {
      var self = this;

      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:promotion_menu:sub_menu'} );

      var workerFilterFn = function(person) {
         return person['stonehearth:profession'].profession_uri == 'stonehearth:professions:worker';
      };

      App.gameView.addView(App.StonehearthPeoplePickerView, {
                  title: 'Choose the worker to promote', //xxx localize
                  position: {
                     my: 'center center',
                     at: 'center center',
                     of: self.$('#scroll'),
                  },
                  // A user-specified function to filter the citizens that come back from
                  // the server.
                  filterFn: workerFilterFn,
                  // Additional components of the citizen entity that we want retrieved
                  // (for our function).
                  additional_components: { "stonehearth:profession" : {} },
                  callback: function(person) {
                     // `person` is part of the people picker's model.  it will be destroyed as
                     // soon as that view is destroyed, which will happen immediately after the
                     // callback is fired!  scrape everything we need out of it before this happens.
                     radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:promotion_menu:select'} );
                     self.set('citizen', person);
                     self.showApproveStamper();
                  }
               });
   },

   approve: function() {
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:promotion_menu:stamp'} );
      var self = this;

      self.promote();

      // animate down
      self.$('#approveStamper').animate({ bottom: 20 }, 130 , function() {
         self.$('#approvedStamp').show();
         //animate up
         $(this)
            .delay(200)
            .animate({ bottom: 200 }, 150, function () {
               // close the wizard after a short delay
               setTimeout(function() {
                  self.invokeDestroy();
               }, 1500);
            });
         });
   },

   promote: function() {
      var self = this;
      var citizen = this.get('citizen');
      var talisman = this.get('talismanUri');

      if (!talisman) {
         talisman = this.get('talisman').__self;
      }

      radiant.call('stonehearth:grab_promotion_talisman', citizen.__self, talisman);
   },

   // iterate over the stored items and unlock jobs where the player has the required talisman
   unlockJobs: function(professions) {
      var self = this;
      var professions = this._getAvailableProfessions(professions);

      //xxx, todo, keep the last 2 versions of the talismans, so we can compare them and highlight
      //which buttons are new since the last opening of the window
      this.$('.jobButton').addClass('locked');
      this.$('.jobButton').each(function() {
         var alias = $(this).attr('id');
         if (professions[alias]) {
            $(this).removeClass('locked');   
         }
      })
   },

   _getAvailableProfessions: function(professions) {
      var availableProfessions = {};

      $.each(professions, function(key, profession) {
         availableProfessions[profession] = true;
      });

      return availableProfessions;
   },

   getProfessionInfo: function(id) {
      var ret;
      $.each(this.get('context.jobs'), function(i, job) {
         if (job.alias == id) {
            ret = job;
         }
      })

      return ret;
   },

   showApproveStamper: function() {
      var stamp = self.$('#approveStamper');
      stamp.show();
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

});