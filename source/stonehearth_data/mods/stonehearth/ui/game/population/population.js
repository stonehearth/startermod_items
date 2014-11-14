var StonehearthPopulation;

(function () {
   StonehearthPopulation = SimpleClass.extend({

      _components: {
         "citizens" : {
            "*" : {
               "unit_info": {},           
               "stonehearth:commands": {
                  commands: []
               },
               "stonehearth:ai" : {},
               "stonehearth:job" : {
                  "job_uri" : {}
               },
               "stonehearth:crafter" : {
                  "workshop" : {}
               },
            }
         }
      },

      init: function(initCompletedDeferred) {
         var self = this;
         this._initCompleteDeferred = initCompletedDeferred;

         radiant.call('stonehearth:get_population')
            .done(function(response){
               self._populationUri = response.population;
               self._initCompleteDeferred.resolve();
            });
      },

      getUri: function() {
         return this._populationUri;
      },

      getComponents: function() {
         return this._components;
      },

      getTrace: function() {
         return new StonehearthPopulationTrace();
      }
   });

   StonehearthDataTrace = SimpleClass.extend({

      init: function(uri, components) {
         if (uri) {
            this._uri = uri;   
         }

         if (components) {
            this._components = components;   
         }

         this._radiantTrace = new RadiantTrace();
         this._deferred = this._radiantTrace.traceUri(this._uri, this._components);
      },

      progress: function(fn) {
         this._deferred.progress(fn);
         return this;
      },

      done: function(fn) {
         this._deferred.done(fn);
         return this;
      },

      fail: function(fn) {
         this._deferred.fail(fn);
         return this;
      },

      destroy: function() {
         if (this._radiantTrace) {
            this._radiantTrace.destroy();
            this._radiantTrace = null;            
         }
         return this;
      }

   });

   StonehearthPopulationTrace = StonehearthDataTrace.extend({

      init: function() {
         this._uri = App.population.getUri();
         this._components = App.population.getComponents();
         this._super();
      }
   });


})();
