var StonehearthTutorialManager;

(function () {
   StonehearthTutorialManager = SimpleClass.extend({

      init: function() {
         var self = this;

         $(top).on('stonehearth_tutorial_step_finished', function(e, data) {
            self.finishStep(data.tutorial, data.step);
         })
      },

      _tutorials : {
         'starting' : {
            'steps' : [
               'spawnScenario',
               'harvestTree',
               'dingHarvestTree',
               'createStockpile',
            ]
         },
         'buildingEditor' : {
            'steps' : [
               'openEditor',
               'makeBuilding',
            ]
         },
         'promoteCarpenter' : {
            'steps' : [
               'spawnScenario',
               'openCitizensPane',
               'promoteCitizen',
               'buildWorkshop',
               'craftHoe',
            ]
         }
      },

      start: function(tutorialName) {
         var self = this;
         if (!tutorialName) {
            tutorialName = 'starting'
         }

         var tutorial = self._tutorials[tutorialName];

         self._startStep(tutorialName, tutorial.steps[0]);

         /*
         radiant.call('stonehearth:get_client_service', 'tutorial')
            .done(function(e) {
               self._tutorial = e.result;

               radiant.trace(self._tutorial)
                  .progress(function(change) {
                     self.doTutorialStep(change.current_step);
                  });
            })
         */

         //self._showDingStep('starting', 'spawnScenario', 'foo', 'foobar');
      },

      _startStep: function(tutorialName, step) {
         var self = this;
         var tutorial = self._tutorials[tutorialName];

         if (tutorial && $.inArray(step, tutorial.steps) != -1 && step != tutorial['_currentStep']) {
            self._setCurrentStap(tutorialName, step)

            if (self._tutorialSteps[tutorialName] && self._tutorialSteps[tutorialName][step]) {               
               self._tutorialSteps[tutorialName][step]();   
            }
         }
      },

      finishStep: function(tutorialName, step) {
         var self = this;
         var tutorial = self._tutorials[tutorialName];
         if (!tutorial.finishedSteps) {
            tutorial.finishedSteps = {};
         }

         tutorial.finishedSteps[step] = true;

         this._tryNextStep(tutorialName);

         //xxx save
      },

      _getCurrentStep: function(tutorialName) {
         var self = this;
         var tutorial = self._tutorials[tutorialName];

         if (!tutorial._currentStep) {
            this._setCurrentStap(tutorial.steps[0]);
         }

         return tutorial._currentStep;
      },

      _setCurrentStap: function(tutorialName, step) {
         var self = this;
         var tutorial = self._tutorials[tutorialName];

         tutorial._currentStep = step;
      },

      _tryNextStep: function(tutorialName) {
         var self = this;

         var tutorial = self._tutorials[tutorialName];
         var currentStep = this._getCurrentStep(tutorialName);

         if (!tutorial.finishedSteps[currentStep]) {
            // the current step hasn't been completed. bail.
         }

         for(i = 0; i < tutorial.steps.length; i++) {
            var stepName = tutorial.steps[i];

            if (!tutorial.finishedSteps[stepName]) {
               // found a step that hasn't been comleted. start that step and break
               self._startStep(tutorialName, stepName);
               break;
            }
         }
      },

      // ----- All the tutorial steps ---
      _tutorialSteps: {
         starting: {
            spawnScenario: function() {
              radiant.call('stonehearth:spawn_scenario', 'stonehearth:quests:collect_starting_resources');
            },

            harvestTree: function() {
               App.stonehearthTutorials._highlightElement('#harvest_menu', 'Click the Harvest button to gather resources');

               // wait for wood to hit the terrain
               radiant.call('stonehearth:get_service', 'tutorial')
                  .done(function(e) {
                     var tutorial_service = e.result;
                     radiant.call_obj(tutorial_service, 'get_first_wood_harvest')
                        .done(function(e) {
                           App.stonehearthTutorials.finishStep('starting', 'harvestTree');
                        })
                        .fail(function(e) {
                           console.dir(e)
                        })
                  })
            },

            dingHarvestTree: function() {
               App.stonehearthClient.hideTip();
               App.stonehearthTutorials._showDingStep('starting', 'dingHarvestTree', 'We\'ve got wood!', 'Next we need someplace to store it');
            },

            createStockpile: function() {
               // click the zones button
               App.stonehearthTutorials._highlightElement('#zone_menu', 'Click the Zones button', 'Zones define what happens where in your town')
                  .done(function() {
                     // click the create stockpile button
                     App.stonehearthTutorials._highlightElement('#create_stockpile', 'Click the Create Stockpile button', 'Workers store your items in stockpiles')
                        .done(function() {
                           // click the OK button
                           App.stonehearthTutorials._highlightElement('#stockpileOk', 'Click the check to confirm')
                              .done(function() {
                                 App.stonehearthTutorials.finishStep('starting', 'createStockpile');
                                 App.stonehearthTutorials._highlightElement('#harvest_menu', 'Harvest some more trees so we can start building');

                                 radiant.call('radiant:set_config', 'tutorial', { hideStartingTutorial: true});
                                 //App.stonehearthTutorials.start('promoteCarpenter');
                              });
                        });
                  });
            },
         },
         buildingEditor: {
            start: function() {

            },

            openEditor: function() {

            },
         },
         promoteCarpenter: {
            spawnScenario: function() {
               radiant.call('stonehearth:spawn_scenario', 'stonehearth:quests:promote_carpenter');  
            },

            openCitizensPane: function() {
               //xxx, check if the citizens pane is already up!
               App.stonehearthTutorials._highlightElement('#citizen_manager', 'Click the Citizens button')
                  .done(function() {
                     App.stonehearthTutorials._highlightElement('.populationContainer table', 'Choose a citizen to promote', 'Use this pane to manage all your citizens')
                        .done(function() {
                           App.stonehearthTutorials._highlightElement('#promote_to_profession button', 'Click the Promote button')
                              .done(function() {
                                 App.stonehearthTutorials._highlightElement('#stonehearth\\:professions\\:carpenter', 'Click the Carpenter button')
                                    .done(function() {
                                       App.stonehearthTutorials._highlightElement('#approveStamper', 'Click the stamper to finish the job!')
                                          .done(function() {
                                             App.stonehearthClient.hideTip();
                                             App.stonehearthTutorials.finishStep('promoteCarpenter', 'openCitizensPane'); 
                                          })
                                    })
                              })
                        })
                  })
            }

         }
      },

      _highlightElement: function(selector, text, description) {
         var self = this;
         var highlightDonDeferred = $.Deferred();


         self._waitForDomElement(selector)
            .done(function() {
               if (self._highlight) {
                  self._highlight.destroy();
                  self._highlight = null;
               }

               self._highlight = App.gameView.addView(App.StonehearthElementHighlight, 
                  { 
                     elementToHighlight : selector,
                     helpText: text,
                     helpDescription: description,
                     completeDeferred: highlightDonDeferred
                  });

            })

         return highlightDonDeferred;
      },

      _waitForDomElement: function(selector) {
         var self = this;
         var domElementDeferred = $.Deferred();

         var wait;
         wait = function() {
            if ($(selector).length > 0) {
               domElementDeferred.resolve();
            } else {
               setTimeout(wait, 1000)
            }            
         }

         wait();

         return domElementDeferred;
      },

      _showDingStep: function(tutorialName, stepName, title, message) {
         var self = this;
         App.gameView.addView(App.StonehearthCongratsPopup, 
            { 
               title : title,
               message : message,
               buttons : [
                  { 
                     label: "Ok",
                     click: function() {
                        self.finishStep(tutorialName, stepName);
                     }
                  }
               ],
               onDestroy : function() {
                  self.finishStep(tutorialName, stepName);
               }
            });         
      }
   });
})();
