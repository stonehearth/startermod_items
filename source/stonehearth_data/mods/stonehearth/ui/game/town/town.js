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
      this.journalData.initialized = false;
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

      radiant.call('stonehearth:get_journals')
         .done(function(response){
            var uri = response.journals;
            
            if (uri == undefined) {
               //we don't have journals yet. Return
               //Since journals don't live update, it's OK to not listen for them. 
               return;
            }

            self.radiantTraceJournals  = new RadiantTrace()
            self.traceJournals = self.radiantTraceJournals.traceUri(uri, {});
            self.traceJournals.progress(function(eobj) {
                  self.journalData.data = eobj;
                  if (eobj.journals_by_page.length > 0) {
                     if (!self.journalData.initialized) {
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
      if (this.radiantTrace != undefined) {
         this.radiantTrace.destroy();
      }
      if (this.radiantTraceJournals != undefined) {
         this.radiantTraceJournals.destroy();
      }
      this._super();
   },

   didInsertElement: function() {
      var self = this;
      this._super();
      this._updateScores();

      this.$('.tab').click(function() {
         var tabPage = $(this).attr('tabPage');

         self.$('.tabPage').hide();
         self.$('.tab').removeClass('active');
         $(this).addClass('active');

         self.$('#' + tabPage).show();
      });
   },

   //Page 1 of the book is a title page
   //Open to at least page 2, or the most recent praise page
   _bookInit: function(bookPage) {
      var self = this;
      this.$('.book').turn({
                     display: 'double',
                     acceleration: true,
                     gradients: true,
                     elevation:50,
                     page: bookPage,
                     turnCorners: "",
                     when: {
                        turned: function(e, page) {
                           console.log('Current view: ', $(this).turn('view'));
                        }
                     }
                  });

      $(".book").bind("turned", function(event, page, view) {
         if (page > 1) {
            var date = self._deriveDateFromPage(page);
            self._setDate(date);
         }
      });

      this.$('.book').on( 'click', '.odd', function() {
        self._turnForward();
      });

      this.$('.book').on( 'click', '.even', function() {
         self._turnBack();
      });
   },

   //Town related stuff

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
   
   //Take the journal data, make a bunch of pages from it
   //Put those pages under the right parent to make the book
   //Clean out the old pages before appending the new ones.
   _populatePages: function() {
      var journalsByPage = this.journalData.data.journals_by_page;
      
      var allPages = '<div><div><div id="bookBeginning"><h2>' + App.stonehearthClient.settlementName() +  '</h2><p>' +  i18n.t('stonehearth:journal_town_log') + '</p></div></div></div>';

      for(var i=0; i < journalsByPage.length; i++) {
         var entries = journalsByPage[i];
         var isGripes = i%2 == 1;
         var allEntries = this._make_page(entries, isGripes);
         var header = '<div><div><h2 class="praiseTitle ">' +  i18n.t('stonehearth:journal_praises') + '</h2>';
         if (isGripes) {
            header = '<div><div><h2 class="gripeTitle">' +  i18n.t('stonehearth:journal_gripes') + '</h2>';
         }

         allPages += header + allEntries + '</div></div>';
      }
      
      this.$(".book").empty();
      this.$(".book").append(allPages);

      //Make sure we open to the last page. The array starts counting at 0, but
      //the book counts pages starting at 1, and the first page is a bogus title page.
      var lastLeftPageIndex = journalsByPage.length-2;  
      var bookPage = lastLeftPageIndex + 2;
      var pageDate = this._deriveDateFromPage(bookPage);
      this._setDate(pageDate);
      this._bookInit(bookPage);
   },

   _make_page: function(entries, isGripe) {
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
         if (isGripe) {
            allEntries = i18n.t('stonehearth:gripe_empty');
         } else {
            allEntries = i18n.t('stonehearth:praise_empty');
         }
      }
      return allEntries;   
   },

   _deriveDateFromPage: function(pageNumber) {
      var journalsByPage = this.journalData.data.journals_by_page;
      var assocPageIndex = pageNumber - 2;
      var pageDate = "Today";

      if (journalsByPage[assocPageIndex][0] != undefined) {
         pageDate = journalsByPage[assocPageIndex][0].date;
      } else if (journalsByPage[assocPageIndex + 1] != undefined) {
         pageDate = journalsByPage[assocPageIndex + 1][0].date;
      } else if (this.$("#journalDate").html() != "") {
         pageDate = this.$("#journalDate").html()
      }
      return pageDate;
   },

   _setDate: function(date) {
      this.$("#journalDate").html(date.charAt(0).toUpperCase() + date.slice(1));
   },
   
   _turnForward: function() {
      $('.book').turn('next');
   },

   _turnBack: function() {
      var page = $(".book").turn("page");

      // never go to page 1
      if (page > 3) {
         $('.book').turn('previous');   
      }
   },

   actions: {
      back: function() {
         this._turnBack();
      },
      forward: function() {
         this._turnForward();
      }
   }

});

