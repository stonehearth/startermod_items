var StonehearthTool;

(function () {
   StonehearthTool = SimpleClass.extend({

      init: function() {
         this._deferred = new $.Deferred();
      },

      toolFunction: function(toolFunction) {
         this._toolFunction = toolFunction;
         return this
      },

      tooltipSequence: function(tooltipSequence) {
         this._tooltipSequence = tooltipSequence;
         return this;
      },

      sound: function(sound) {
         this._sound = sound;
         return this;
      },

      loops: function(loops) {
         this._loops = loops;
         return this;
      },

      preCall: function(preCall) {
         this._preCall = preCall
         return this;
      },

      done: function(done) {
         this._deferred.done(done);
         return this;
      },

      progress: function(progress) {
         this._deferred.progress(progress);
         return this;
      },

      always: function(always) {
         this._deferred.always(always);
         return this;
      },

      fail: function(fail) {
         this._deferred.fail(fail);
         return this;
      },

      run: function() {
         var self = this;
         var iteration = 0;
         var runArgs = arguments;

         // play the tool sound
         if (self._sound) {
            radiant.call('radiant:play_sound', {'track' : self._sound} );   
         }

         // activate this tool
         var activateTool = function() {
            // run any defined pre calls
            if (self._preCall) {
               self._preCall();
            }

            // show the correct tooltip
            self._showTooltip(iteration);

            // keep track of the active tool
            StonehearthTool._activeTool = self;

            self._toolFunction.apply(this, runArgs)
               .done(function(response) {
                  StonehearthTool._activeTool = null;
                  
                  if (self._loops) {
                     self._deferred.notify(response);
                  } else {
                     self._deferred.resolve(response);
                  }
                  
                  if (self._loops) {
                     iteration++;
                     activateTool();
                  } else {
                     App.stonehearthClient.hideTip();
                  }
               })
               .fail(function(response) {
                  if (self._tooltipSequence) {
                     App.stonehearthClient.hideTip();
                  }

                  StonehearthTool._activeTool = null;
                  self._deferred.reject(response);
               })

         };

         if (StonehearthTool._activeTool) {
            // If we have an active tool, trigger a deactivate so that when that
            // tool completes, we'll activate the new tool.            
            StonehearthTool._activeTool.always(activateTool);
            App.stonehearthClient.deactivateAllTools();
            
         } else {
            activateTool(arguments);
         }

         return this._deferred;
      },

      _showTooltip: function(iteration) {
         if (!this._tooltipSequence) {
            return;
         }

         var tip = Math.min(iteration, this._tooltipSequence.length - 1);
         
         if (typeof this._tooltipSequence[tip] == 'string') {
            App.stonehearthClient.showTip(this._tooltipSequence[tip]);   
         } else {
            App.stonehearthClient.showTip(this._tooltipSequence[tip].title, this._tooltipSequence[tip].description);   
         }
         
      }
   });

})();
