// The view that shows a list of citizens and lets you promote one
App.StonehearthTownView = App.View.extend({
   templateName: 'town',
   classNames: ['flex', 'fullScreen', 'exclusive'],
   closeOnEsc: true,

   journalData: {
      'initialized' : false,
      'data' : null
   },

   init: function() {
      var self = this;
      this.journalData.initialized = false;
      this._super();
      self.set('context.town_name', App.stonehearthClient.settlementName())

      this._popTrace = App.population.getTrace();
      this._popTrace.progress(function(pop) {
            var numWorkers = 0;
            var numCrafters = 0;
            var numSoldiers = 0;
            radiant.each(pop.citizens, function(k, citizen) {
               if (!citizen['stonehearth:job']) {
                  return;
               }

               var roles = citizen['stonehearth:job']['job_uri']['roles'];

               if (roles && roles.indexOf('crafter') > -1) {
                  numCrafters++;
               } else if (roles && roles.indexOf('combat') > -1) {
                  numSoldiers++;
               } else {
                  numWorkers++;
               }
            });

            self.set('context.num_workers', numWorkers);
            self.set('context.num_crafters', numCrafters);
            self.set('context.num_soldiers', numSoldiers);
         });

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
      if (this._popTrace) {
         this._popTrace.destroy()
      }

      if (this.radiantTrace) {
         this.radiantTrace.destroy();
      }

      if (this.radiantTraceJournals) {
         this.radiantTraceJournals.destroy();
      }

      if (this._playerInventoryTrace) {
         this._playerInventoryTrace.destroy();
      }

      this._super();
   },

   didInsertElement: function() {
      var self = this;
      this._super();

      var rows = self.$('#scores .row').each(function( index ) {
         var row =  $( this );
         var scoreName = row.attr('id');
         var tooltipString = App.tooltipHelper.getScoreTooltip(scoreName, true); // True for town description.
         if (tooltipString) {
            row.tooltipster({
                  content: $(tooltipString)
               });
         }
      });

      this._updateUi();

      //inventory tab
      this._inventoryPalette = this.$('#inventoryPalette').stonehearthItemPalette({
         cssClass: 'inventoryItem',
      });

      radiant.call_obj('stonehearth.inventory', 'get_item_tracker_command', 'stonehearth:basic_inventory_tracker')
         .done(function(response) {
            self._playerInventoryTrace = new StonehearthDataTrace(response.tracker, {})
               .progress(function(response) {
                  self._inventoryPalette.stonehearthItemPalette('updateItems', response.tracking_data);
               });
         })
         .fail(function(response) {
            console.error(response);
         });

      this.$('#overviewTab').show();
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

   _updateUi: function() {
      var self = this;
      
      var overallMoral = self.get('context.score_data.aggregate.happiness')

      // Update town label.
      var settlementSize = i18n.t('stonehearth:' + netWorthLevel);
      if (settlementSize != 'stonehearth:undefined') {
         $('#descriptor').html(i18n.t('stonehearth:town_description', {
               "descriptor": i18n.t('stonehearth:' + Math.floor(overallMoral/10) + '_score'), 
               "noun": settlementSize
            }));
      }

      // Update net worth
      var netWorthLevel = self.get('context.score_data.net_worth.level')

      // Update moral scores.
      var scoresToUpdate = ['happiness', 'food', 'shelter', 'safety'];
      radiant.each(scoresToUpdate, function(i, score_name) {
         var score_value = self.get('context.score_data.aggregate.' + score_name);
         self.set('score_' + score_name, Math.round(score_value) / 10);
         self._setIconClass(score_name + 'IconClass', score_value);
      });
   },

   _setIconClass: function(className, value) {
      var iconValue = Math.floor(value / 10); // value between 1 and 10
      this.set(className, 'happiness_' + iconValue);
   },

   _observerScores: function() {
      this._updateUi();
   }.observes('context.score_data.happiness'),

   _updateMeter: function(element, value, text) {
      element.progressbar({
         value: value
      });

      element.find('.ui-progressbar-value').html(text.toFixed(1));
   },

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
         var allEntries = this._makePage(entries, isGripes);
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

   _makePage: function(entries, isGripe) {
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
      var pageDate = i18n.t('stonehearth:uneventful_day');

      if (journalsByPage[assocPageIndex][0] != undefined) {
         pageDate = journalsByPage[assocPageIndex][0].date;
      } else {
         //Check the adjacent page. Which is us -1 if we are odd, and us plus one if we're even
         if (assocPageIndex%2 == 1) {
            assocPageIndex--;
         } else {
            assocPageIndex++;
         }
         if (journalsByPage[assocPageIndex][0] != undefined) {
            pageDate = journalsByPage[assocPageIndex][0].date;
         } 
      }
      return pageDate;
   },

   _setDate: function(date) {
      //var targetText = 
      //if (date != null) {

      //}
      this.$("#journalDate").html(date.charAt(0).toUpperCase() + date.slice(1));
   },
   
   _turnForward: function() {
      var page = $(".book").turn("page");

      if ( page != undefined ) {
         $('.book').turn('next');
      }
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

