App.StonehearthPartiesView = App.View.extend({
	templateName: 'parties',
   uriProperty: 'model',
   closeOnEsc: true,

   didInsertElement: function() {
      var self = this;  
      this._super();

      radiant.call_obj('stonehearth.party_editor', 'show_party_banners_command', true);

      // remember the party for the row that the mouse is over
      this.$().on('mouseenter', '.row', function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_hover' }); // Mouse over SFX
      });

      // show toolbar control
      this.$().on('click', '.row', function() {  
         var selected = $(this).hasClass('selected'); // so we can toggle!
         self.$('.row').removeClass('selected');
         
         if (!selected) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_click' });  // selecting which party you want SFX 
            $(this).addClass('selected');
         }
      });
   },

   destroy: function() {
      this._super();
      radiant.call_obj('stonehearth.party_editor', 'show_party_banners_command', false);
   },

   actions: {
      createParty: function(command) {
         radiant.call_obj('stonehearth.unit_control', 'create_party_command')
                  .done(function(response) {
                     console.log(response)
                     App.stonehearthClient.showPartyEditor(response.party);
                  })
                  .fail(function(response) {
                     console.log('failed to create party', response)
                  })
      },
   },

   init: function() {
      var self = this;
      this._super();
      this.set('title', 'Parties');

      radiant.call_obj('stonehearth.unit_control', 'get_controller_command')
               .done(function(response) {
                  self.set('uri', response.uri)
               });
   },

   partiesArray: function() {
      return radiant.map_to_array(this.get('model.parties'));
   }.property('model.parties'),
});

App.StonehearthPartiesRowView = App.View.extend({
   tagName: 'tr',
   classNames: ['row'],
   templateName: 'partiesRow',
   uriProperty: 'model',
   closeOnEsc: true,

   click: function(evt) {
      var party = this.get('model')
      App.stonehearthClient.showPartyEditor(party.__self);
   },

   actions: {
      setAttackOrder: function(party) {
         App.stonehearthClient.placePartyBanner(party, 'attack');
      },
      setDefendOrder: function(party) {
         App.stonehearthClient.placePartyBanner(party, 'defend');
      },
   },
});
