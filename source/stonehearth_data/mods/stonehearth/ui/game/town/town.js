// The view that shows a list of citizens and lets you promote one
App.StonehearthTownView = App.View.extend({
   templateName: 'town',
   classNames: ['flex', 'fullScreen'],
   closeOnEsc: true,

   scores: {
      'net_worth_percent' : 10,
      'net_worth_total': 0,
      'net_worth_level' : 'camp',
      'happiness' : 50, 
      'nutrition' :50, 
      'shelter' : 50
   },

   journalData: {
      'initialized' : false,
      'data' : null
   },

   init: function() {
      var self = this;
      this.initialized = false;
      this._super();
      self.set('context.town_name', App.stonehearthClient.settlementName())

      radiant.call('stonehearth:get_score')
         .done(function(response){
            var uri = response.score;
            
            self.radiantTrace  = new RadiantTrace()
            self.trace = self.radiantTrace.traceUri(uri, {});
            self.trace.progress(function(eobj) {
                  self.set('context.score_data', eobj.score_data);
               });
         });

         
      this.currJournalIndex = null;
      this.set('context.currJournalDayData', {});
      this.set('context.currJournalDayData.score_up', []);
      this.set('context.currJournalDayData.score_down', []);
      this.set('context.currJournalDayData.date', 'Today');

      radiant.call('stonehearth:get_journals')
         .done(function(response){
            var uri = response.journals;
            
            self.radiantTraceJournals  = new RadiantTrace()
            self.traceJournals = self.radiantTraceJournals.traceUri(uri, {});
            self.traceJournals.progress(function(eobj) {
                  self.journalData.data = eobj;
                  if (eobj.journals_by_page.length > 0) {
                     if (!self.journalData.initialized) {
                        //TODO: make the journal with the entries available. 
                        //TODO: What if the game has been going for 3 in game years? Would we make a book of a thousand pages?
                        //TODO: consider culling to just the last N entries/pages
                        self.journalData.initialized = true;
                        self._populatePages();
                     } 
                  }
               });
         });

   },

   destroy: function() {
      this.radiantTrace.destroy();
      this.radiantTraceJournals.destroy();
      this._super();
   },

   didInsertElement: function() {
      var self = this;
      this._super();
      this._updateScores();

      // close button handler
      this.$('.title .closeButton').click(function() {
         self.destroy();
      });

      this.$('.tab').click(function() {
         var tabPage = $(this).attr('tabPage');

         self.$('.tabPage').hide();
         self.$('.tab').removeClass('active');
         $(this).addClass('active');

         self.$('#' + tabPage).show();
      });
   },

   _bookInit: function() {
      // add turn page effect
      this.$('.book').turn({
                     display: 'double',
                     acceleration: true,
                     gradients: true,
                     elevation:50,
                     page: 2,
                     turnCorners: "",
                     when: {
                        turned: function(e, page) {
                           console.log('Current view: ', $(this).turn('view'));
                        }
                     }
                  });


      this.$('.book').on( 'click', '.odd', function() {
         $('.book').turn('next');
      });

      this.$('.book').on( 'click', '.even', function() {
         var page = $(".book").turn("page");

         // never go to page 1
         if (page > 2) {
            $('.book').turn('previous');   
         }
      });
   },

   _update_town_label: function() {
      var happiness_score_floor = Math.floor(this.scores.happiness/10);
      var settlement_size = i18n.t('stonehearth:' + this.scores.net_worth_level);
      if (settlement_size != 'stonehearth:undefined') {
         $('#descriptor').html(i18n.t('stonehearth:town_description', {
               "descriptor": i18n.t('stonehearth:' + happiness_score_floor + '_score'), 
               "noun": settlement_size
            }));
      }
   },

   _updateScores: function() {
      this.$('#netWorthBar').progressbar({
          value: this.scores.net_worth_percent
      });

      this._updateMeter(this.$('#overallScore'), this.scores.happiness, this.scores.happiness / 10);
      this._updateMeter(this.$('#foodScore'), this.scores.nutrition, this.scores.nutrition / 10);
      this._updateMeter(this.$('#shelterScore'), this.scores.shelter, this.scores.shelter / 10);

      this._update_town_label();
   },

   _updateMeter: function(element, value, text) {
      element.progressbar({
         value: value
      });

      element.find('.ui-progressbar-value').html(text.toFixed(1));
   },

   _set_happiness: function() {
      this.scores.happiness = this.get('context.score_data.happiness.happiness');
      this.scores.nutrition = this.get('context.score_data.happiness.nutrition');
      this.scores.shelter = this.get('context.score_data.happiness.shelter');
      this._updateScores();
   }.observes('context.score_data.happiness'),

   _set_worth: function() {
      this.scores.net_worth_percent = this.get('context.score_data.net_worth.percentage');
      this.scores.net_worth_total = this.get('context.score_data.net_worth.total_score');
      this.scores.net_worth_level = this.get('context.score_data.net_worth.level');
      this._updateScores();
   }.observes('context.score_data.net_worth'),


   //Journal related stuff
   _changeJournalIndex: function(nextIndex) {
      this.currJournalIndex = nextIndex;
      this.set('context.currJournalDayData', this.journalData.data.journals_by_date[nextIndex]);

      //If the arrays come over empty, for some reason ember thinks they are objects and freaks out
      if (this.journalData.data.journals_by_date[nextIndex].score_up.length == undefined) {
          this.set('context.currJournalDayData.score_up', []);
      }
      if (this.journalData.data.journals_by_date[nextIndex].score_down.length == undefined) {
          this.set('context.currJournalDayData.score_down', []);
      }
   },

   
   //Take the journal data, make a bunch of pages from it
   //Put those pages under the right parent to make the book
   //Clean out the old pages before appending the new ones.
   _populatePages: function() {
      var journalsByPage = this.journalData.data.journals_by_page;
      
      var allPages = '<div><div><div id="bookBeginning">' + App.stonehearthClient.settlementName() +  '<p>Town Log</p></div></div></div>';

      for(var i=0; i < journalsByPage.length; i++) {
         var entries = journalsByPage[i];
         var allEntries = this._make_page(entries);
         var header = '<div><div><h2 class="praiseTitle ">Praises</h2>';
         if (i%2 == 1) {
            header = '<div><div><h2 class="gripeTitle">Gripes</h2>';
         }

         allPages += header + allEntries + '</div></div>';
      }
      
      this.$(".book").empty();
      this.$(".book").append(allPages);

      this._bookInit();
   },

   _make_page: function(entries) {
      var allEntries = "";
      if (entries.length != undefined && entries.length > 0) {
         for(var j=0; j<entries.length; j++) {
            var entry = entries[j];
            allEntries += '<p class="logEntry">' + 
                    '<span class="logTitle">' + entry.title + '</span></br>' +
                    entry.text + '</br>' +
                    '<span class="signature">-- ' + entry.person_name + '</span>' +
                    '</p>';
         }
      }
      if (allEntries == "") {
         allEntries = "Nothing interesting happened today.";
      }
      return allEntries;   
   },
   
   /*
   //TODO: Restore if we want to try to update individual pages as more data comes in
   //TODO: make sure to account for updating 
   _update_last_page: function() {
      var $lastPage = this.$('.entries:last');
      var lastIndex = this.journalData.data.journals_by_page.length;
      var entries = this.journalData.data.journals_by_page[lastIndex-1];
      var allEntries = this._make_page(entries);
      $lastPage.empty();
      $lastPage.append(allEntries);
   },
   */

   actions: {
      back: function() {
         if (this.currJournalIndex + 1 < this.journalData.data.journals_by_date.length) {
            this._changeJournalIndex(this.currJournalIndex + 1);
         }
      },
      forward: function() {
         if (this.currJournalIndex != 0) {
            this._changeJournalIndex(this.currJournalIndex - 1);
         }
      }
   }

});

