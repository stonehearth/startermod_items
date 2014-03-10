var StonehearthClient;

(function () {
   StonehearthClient = SimpleClass.extend({

      init: function() {
         var self = this;

         radiant.call('stonehearth:get_build_editor')
            .done(function(e) {
               self._build_editor = e.build_editor;
            })
            .fail(function(e) {
               console.log('error getting build editor')
               console.dir(e)
            })
      },

      getActiveTool: function() {
         return this._activeTool;
      },

      deactivateAllTools: function() {
         var self = this;
         return radiant.call('radiant:client:deactivate_all_tools')
            .always(function() {
               self._activeTool = null;
            });
      },

      // Wrapper to call all tools, handling the boilerplate tool management.
      _callTool: function(toolFunction) {
         var self = this;

         var deferred = new $.Deferred();

         this.deactivateAllTools()
            .always(function() {
               // when all tools are deactivated, activate the tool specified in the params
               self._activeTool = toolFunction()
                  .done(function(response) {
                     deferred.resolve(response);
                  })
                  .fail(function(response) {
                     deferred.reject(response);
                  })
                  .always(function (){
                     self._activeTool = null;
                  })
            });

         return deferred;
      },

      createStockpile: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
         
         // xxx, localize
         $(top).trigger('radiant_show_tip', { 
            title : 'Click and drag to create your stockpile',
            description : 'Your citizens place resources and crafted goods in stockpiles for safe keeping!'
         });

         return this._callTool(function() {
            return radiant.call('stonehearth:choose_stockpile_location')
               .done(function(response) {
                  // XXX, instead of this, we should select the entity using radiant.client
                  $(top).trigger('select_stockpile', response.stockpile);
               })
               .always(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  $(top).trigger('radiant_hide_tip');
                  console.log('stockpile created!');
               });
         })
      },

      buildWall: function() {
         var self = this;

         $(top).trigger('radiant_show_tip', { 
            title : 'Click to place wall segments',
            description : 'Hold down SHIFT while clicking to draw connected walls!'
         });

         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'place_new_wall')
               .always(function(response) {
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      buildRoom: function() {
         var self = this;
         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'create_room');
         });
      },

   });

})();

App.stonehearthClient = new StonehearthClient();
