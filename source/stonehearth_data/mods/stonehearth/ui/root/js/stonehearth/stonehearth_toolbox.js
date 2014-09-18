var StonehearthToolbox;

(function () {
   // bookkeeping class to keep track of the active tool in the game
   StonehearthToolbox = SimpleClass.extend({

      init: function() {
         this._tools = {}
      },

      addTool: function(id) {
         this._tools[id] = 
      }

      toolFunction: function(toolFunction) {
         this._toolFunction = toolFunction;
         return this
      },

      tooltipSequence: function(tooltipSequence) {
         this._tooltipSequence = tooltipSequence;
         return this;
      },

      loops: function(loops) {
         this._loops = loops;
         return this;
      },

      preCall: function(preCall) {
         this._preCall = preCall
         return this;
      }

      run: function() {
         var self = this;
         var deferred = new $.Deferred();

         // activate this tool
         var activateTool = function() {
            // run any defined pre calls
            if (self._preCall) {
               self._preCall();
            }

            // keep track of the active tool
            StonehearthToolbox._activeTool = self;

            // run the tool function
            self._toolFunction()
               .done(function(response) {
                  deferred.resolve(response);
               })
               .fail(function(response) {
                  deferred.reject(response);
               })

         };

         if (blah) {
            // If we have an active tool, trigger a deactivate so that when that
            // tool completes, we'll activate the new tool.
            blah.always(activateTool);
            App.stonehearthClient.deactivateAllTools();            
         } else {
            activateTool();
         }

         return deferred;
      },

      _callTool: function(toolFunction, preCall) {
         var self = this;

         var deferred = new $.Deferred();

         var activateTool = function() {
            if (preCall) {
               preCall();
            }
            self._activeTool = toolFunction()
               .done(function(response) {
                  self._activeTool = null;
                  deferred.resolve(response);
               })
               .fail(function(response) {
                  self._activeTool = null;
                  deferred.reject(response);
               });
         };

         if (self._activeTool) {
            // If we have an active tool, trigger a deactivate so that when that
            // tool completes, we'll activate the new tool.
            self._activeTool.always(activateTool);
            this.deactivateAllTools();
         } else {
            activateTool();
         }

         return deferred;
      },      
   });
})();
