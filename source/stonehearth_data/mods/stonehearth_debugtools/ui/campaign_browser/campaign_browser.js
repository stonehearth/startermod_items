$(document).on('stonehearthReady', function(){
   App.debugDock.addToDock(App.StonehearthCampaignBrowserIcon);
});

App.StonehearthCampaignBrowserIcon = App.View.extend({
   templateName: 'campaignBrowserIcon',
   classNames: ['debugDockIcon'],

   didInsertElement: function() {
      this.$().click(function() {
         App.debugView.addView(App.StonehearthGameMasterView)   
      })
   }
});

App.StonehearthGameMasterView = App.View.extend({
   templateName: 'game_master',
   uriProperty: 'model',
   components: {
      "ctx" : {}
   },

   init: function() {
      var self = this;

      this._super();
      radiant.call_obj('stonehearth.game_master', 'get_data_command')
         .done(function(o) {
            self.set('uri', o.result)
         })
         .fail(function(o) {
            console.log('call to stonehearth.game_master:get_data() failed.', o)
         });
   },

   campaigns: function() {
      return this.get('model.ctx.child_nodes');
   }.property('model.ctx.child_nodes'),
});


App.StonehearthCampaignView = App.View.extend({
   templateName: 'campaign',
   uriProperty: 'model',

   arcs: function() {
      return this.get('model.child_nodes');
   }.property('model.child_nodes', 'model'),
});

App.StonehearthArcView = App.View.extend({
   templateName: 'arc',
   uriProperty: 'model',

   encounters: function() {
      return this.get('model.child_nodes');
   }.property('model.child_nodes'),
});

App.StonehearthEncounterView = App.View.extend({
   templateName: 'encounter',
   uriProperty: 'model',
   
   encounters: function() {
      return this.get('model.child_nodes');
   }.property('model.child_nodes'),
});
