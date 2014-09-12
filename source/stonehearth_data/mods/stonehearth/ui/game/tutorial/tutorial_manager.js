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
               'createStockpile'
            ]
         }
      },

      start: function() {
         var self = this;

         $.each(self._tutorials, function(name, tutorial) {
            self._startStep(name, tutorial.steps[0]);
         })
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
               var self = this;

               //App.stonehearthClient.showTip('Click the Harvest button to gather resources');
               App.stonehearthTutorials._highlightElement('#harvest_menu', 'Click the Harvest button to gather resources', function() {
                  //xxx, this should happen when wood hits the ground
                  App.stonehearthTutorials.finishStep('starting', 'harvestTree');
               });
            },    

            createStockpile: function() {
               App.stonehearthTutorials.showTip('create stockpile step');
            },
         },         
      },

      _highlightElement: function(selector, text, completeFn) {
         var el = $(selector);

         if (this._highlight) {
            this._highlight.destroy();
            this._highlight = null;
         }

         this._highlight = App.gameView.addView(App.StonehearthElementHighlight, 
            { 
               elementToHighlight : el,
               helpText: text,
               completeFn: completeFn
            });
      }
   });
})();
