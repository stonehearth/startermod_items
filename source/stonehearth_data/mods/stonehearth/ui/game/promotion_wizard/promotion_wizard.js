$(document).ready(function(){
   $(top).on("radiant_promote_to_profession", function (_, e) {
      var r  = new RadiantTrace()
      var components = { 
            'stonehearth:promotion_talisman' : {} 
         };

      // grab the properties from the talisman and pass them along to the promotion wizard
      r.traceUri(e.entity, components)
         .progress(function(eobj) {
               var talisman = eobj;
               App.gameView.addView(App.StonehearthPromotionWizard, { 
                  talisman: talisman,
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

   init: function() {
      var self = this;
      this._super();

      // XXX, this should work! Don't know why I have to manually set the trace below
      //this.set('uri', 'stonehearth:professions:index');

      var r  = new RadiantTrace()
      var trace = r.traceUri('stonehearth:professions:index', this.components);
      trace.progress(function(eobj) {
               self.set('context', eobj);
            });

      $(top).on("radiant_selection_changed.promote_view", function (_, data) {
         var selected = data.selected_entity;
         var pop = App.population.getData();

         $.each(pop.citizens, function(k, citizen) {
            var uri = citizen['__self']
            if (uri == data.selected_entity) {
               var profession = citizen['stonehearth:profession']['profession_uri']['alias'];

               if (profession = 'stonehearth:professions:worker') {
                  self.set('citizen', citizen);
                  self.showApproveStamper();                  
               }
            }
         });
      });

   },

   destroy: function() {
      $(top).off("radiant_selection_changed.promote_view");
      this._super();
   },   

   didInsertElement: function() {
      var self = this;
      this._super();

      var parentView = self.get('parentView');
      parentView.jobsView = this;

      this.$('#jobs').hover( 
         function() {

         },
         function() {
            self.$('#info #name').html(i18n.t('stonehearth:citizens_choose_job_subtitle'));
            self.$('#info #description').html(i18n.t('stonehearth:citizens_choose_job_description'));
         });

      this.$('.jobButton').hover(
         function() {
            var professionInfo = self.getProfessionInfo($(this).attr('id'));
            self.$('#info #name').html(professionInfo.name);
            self.$('#info #description').html(professionInfo.description);
         },
         function() {
            self.$('#info #name').html('');
            self.$('#info #description').html('');
         });

      this.$('.jobButton').click(function() {
         var professionInfo = self.getProfessionInfo($(this).attr('id'));
         self.set('profession', professionInfo);
         self.set('talismanUri', $(this).attr('talisman_uri'));
         self.$('#finishPage').show();
      })

      this.$('#closeButton').click(function() {
         self.destroy();
      });

      this.$('#finishPage #backButton').click(function() {
        self.$('#finishPage').hide(); 
      })

      this.$('#chooseButton').click(function() {
         self.chooseCitizen();
      });

      this.$('#approveStamper').click(function() {
         self.approve();
      });

      self.initWizardState();
   },

   initWizardState: function() {
      // if the talisman is specified
      var talisman = this.get('talisman');
      if (talisman) {
         this.set('profession', talisman.profession);
         self.$('#jobsPage').hide();
         self.$('#finishPage').show();
      }

      // if the citizen is specified
      if (this.get('citizen')) {
         this.showApproveStamper();
      }

      this.unlockJobs();
   },

   chooseCitizen: function() {
      var self = this;

      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:promotion_menu:sub_menu' );

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
                     radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:promotion_menu:select' );
                     self.set('citizen', person);
                     self.showApproveStamper();
                  }
               });
   },

   approve: function() {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:promotion_menu:stamp' );
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
                  self.destroy();
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
   unlockJobs: function() {
      var self = this;
      var talismans = this._getTalismans();

      //xxx, todo, keep the last 2 versions of the talismans, so we can compare them and highlight
      //which buttons are new since the last opening of the window
      this.$('.jobButton').addClass('locked');
      this.$('.jobButton').each(function() {
         var alias = $(this).attr('id');
         if (talismans[alias]) {
            $(this).removeClass('locked');   
         }
      })
   },

   _getTalismans: function() {
      var talismans = {};
      var inventory = App.inventory.getData().items;

      $.each(inventory, function(key, item) {
         var talisman = item['stonehearth:promotion_talisman']
         if (talisman) {
            talismans[talisman.profession] = true;
         }
      });

      return talismans;
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