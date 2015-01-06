App.StonehearthPartyEditorView = App.View.extend({
	templateName: 'partyEditor',
   uriProperty: 'model',
   closeOnEsc: true,

   didInsertElement: function() {
      var self = this;

      this.$('#name').keypress(function(e) {
         if (e.which == 13) {
            var party = self.get('model');
            if (party) {
               radiant.call_obj(party, 'set_name_command', $(this).val())
                        .fail(function(response) {
                           console.log('set name failed', response)
                        });

            }
            $(this).blur();
         }
      });
   },

   actions: {
      editRoster: function(command) {
         if (!this._addMemberView || this._addMemberView.isDestroyed) {
            var party = this.get('model');
            this._addMemberView = App.gameView.addView(App.StonehearthPartyEditorEditRosterView, { 'party' : party.__self });
         } else {
            this._addMemberView.destroy();
            this._addMemberView = null;
         }
      },
   },

   membersArray: function() {
      return radiant.map_to_array(this.get('model.members'), function(k, v) {
         return v.entity;
      });
   }.property('model.members'),
});


App.StonehearthPartyMemberRowView = App.View.extend({
   tagName: 'tr',
   classNames: ['row'],
   templateName: 'partyMemberRow',
   uriProperty: 'model',
   closeOnEsc: true,

   components: {
      "unit_info": {},
      "stonehearth:job" : {
         "job_uri" : {}         
      },
      "stonehearth:party_member" : {
         "party" : {}         
      },
   },

   actions: {
      removePartyMember: function(citizen) {
         var party = this.get('party');
         radiant.call_obj(party, 'remove_member_command', citizen.__self)
                  .fail(function(response) {
                     console.log('failed to remove party member', response);
                  });

      },
   },
});

App.StonehearthPartyEditorEditRosterView = App.View.extend({
   templateName: 'partyEditorEditRoster',
   uriProperty: 'model',
   closeOnEsc: true,

   init: function() {
      var self = this;
      this._super();
      radiant.call_obj('stonehearth.population', 'get_population_command')
               .done(function(response) {
                  self.set('uri', response.uri)
               });
   },

   actions: {
      close: function() {
         this.destroy();
      },
   },

   didInsertElement: function() {
      var self = this;  
      this._super();

      // remember the citizen for the row that the mouse is over
      this.$().on('mouseenter', '.row', function() {
      });
   },

   citizensArray: function() {
      return radiant.map_to_array(this.get('model.citizens'));
   }.property('model.citizens'),
});

App.StonehearthPartyEditorEditRosterRowView = App.View.extend({
   tagName: 'tr',
   classNames: ['row'],
   templateName: 'partyEditorEditRosterRow',
   uriProperty: 'model',
   closeOnEsc: true,

   components: {
      "unit_info": {},
      "stonehearth:job" : {
         "job_uri" : {}         
      },
      "stonehearth:party_member" : {
         "party" : {}         
      },
   },

   actions: {
      addPartyMember: function(citizen) {
         var party = this.get('party');
         radiant.call_obj(party, 'add_member_command', citizen.__self)
                  .fail(function(response) {
                     console.log('failed to add party member', response);
                  });

      },
      removePartyMember: function(citizen) {
         var party = this.get('party');
         radiant.call_obj(party, 'remove_member_command', citizen.__self)
                  .fail(function(response) {
                     console.log('failed to remove party member', response);
                  });

      },
   },

   in_current_party: function() {
      var viewPartyUri = this.get('party');
      var citizenPartyUri = this.get('model.stonehearth:party_member.party.__self');
      return viewPartyUri == citizenPartyUri;
   }.property('model.stonehearth:party_member.party', 'view.party'),

});
