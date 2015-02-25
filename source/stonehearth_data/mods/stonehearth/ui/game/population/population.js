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
         self._numWorkers = 0;
         self._numCrafters = 0;
         self._numSoldiers = 0;

         radiant.call('stonehearth:get_population')
            .done(function(response){
               self._populationUri = response.population;
               self._initCompleteDeferred.resolve();
               self._createGetterTrace();
            });
      },

      getNumWorkers: function() {
         return this._numWorkers;
      },

      getNumCrafters: function() {
         return this._numCrafters;
      },

      getNumSoldiers: function() {
         return this._numSoldiers;
      },

      getUri: function() {
         return this._populationUri;
      },

      getComponents: function() {
         return this._components;
      },

      getTrace: function() {
         return new StonehearthPopulationTrace();
      },

      // easy, instant access to some commonly used variables (e.g. when we need to save a game).
      // NOT useful for implementing a reactive ui!  make your own trace for that!!
      _createGetterTrace: function() {
         var self = this;
         self._getterTrace = new RadiantTrace(self._populationUri, self._components)
            .progress(function(pop) {
               self._numWorkers = 0;
               self._numCrafters = 0;
               self._numSoldiers = 0;

               radiant.each(pop.citizens, function(k, citizen) {
                  if (!citizen['stonehearth:job']) {
                     return;
                  }
                  var roles = citizen['stonehearth:job']['job_uri']['roles'];
                  if (roles && roles.indexOf('crafter') > -1) {
                     self._numCrafters++;
                  } else if (roles && roles.indexOf('combat') > -1) {
                     self._numSoldiers++;
                  } else {
                     self._numWorkers++;
                  }
               });
            });
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
