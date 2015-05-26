App.StonehearthEmbarkView = App.View.extend({
   templateName: 'stonehearthEmbark',
   i18nNamespace: 'stonehearth',

   // Game options (such as peaceful mode, etc.)
   _options: {},
   _components: {
      "citizens" : {
         "*" : {
            "unit_info": {},
            "stonehearth:attributes": {
               "attributes" : {}
            },
         }
      }
   },

   init: function() {
      this._super();
      var self = this;
      this._trace = new StonehearthDataTrace(App.population.getUri(), this._components);

      this._trace.progress(function(pop) {
            self.set('model', pop);
            self._buildCitizensArray();
         });
   },

   didInsertElement: function() {
      this._super();
      var self = this;
      $('body').on( 'click', '#embarkButton', function() {
         self._embark();
      });

      self.$("#regenerateButton").click(function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:reroll'} );

         radiant.call_obj('stonehearth.game_creation', 'generate_citizens_command')
            .done(function(e) {
            })
            .fail(function(e) {
               console.error('generate_citizens failed:', e)
            });
      });

      radiant.call_obj('stonehearth.game_creation', 'generate_citizens_command')
         .done(function(e) {
         })
         .fail(function(e) {
            console.error('generate_citizens failed:', e)
         });

      $(document).on('keydown', this._clearSelectionKeyHandler);
   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();
      }
      $(document).off('keydown', this._clearSelectionKeyHandler);
      this._super();
   },

   _embark: function() {
      var self = this;
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:embark'} );
      App.shellView.addView(App.StonehearthSelectSettlementView, {options: self._options});
      self.destroy();
   },

   _generateCitizenPortrait: function(citizen) {
      var self = this;
      var scene = {
         lights : [
            {
               // Only supports directional lights for now
               color: [0.9, 0.8, 0.9],
               ambient_color: [0.3, 0.3, 0.3],
               direction: [-45, -45, 0]
            },
         ],
         entity_alias: "stonehearth:furniture:comfy_bed",
         camera: {
            // Hmm, did you say you wanted an ortho camera?
            position: [4,3,4],
            look_at: [0,0,0]
         }
      };
      radiant.call_obj('stonehearth.portrait_renderer', 'render_scene_command', scene)
         .done(function(response) {
            $('#testImage').attr('src', 'data:image/png;base64,' + response.bytes);
         })
         .fail(function(e) {
            console.error('generate portrait failed:', e)
         });
   },

   _buildCitizensArray: function() {
      var self = this;
      var citizenMap = this.get('model.citizens');
      var firstCitizen = true;
      var vals = radiant.map_to_array(citizenMap, function(citizen_id ,citizen) {
         citizen.set('__id', citizen_id);
         if (firstCitizen) {
            self._generateCitizenPortrait(citizen.__self);
            firstCitizen = false;
         }
      });
      this.set('model.citizensArray', vals);
   },

});
