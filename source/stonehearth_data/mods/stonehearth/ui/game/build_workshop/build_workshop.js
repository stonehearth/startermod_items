$(document).ready(function(){

   $(top).on("build_workshop.stonehearth", function (_, e) {
      var view = App.gameView.addView(App.StonehearthCrafterBuildWorkshopView, {
         uri: e.event_data.crafter
      });
   });
});

// Expects the uri to be an entity with a stonehearth:workshop
// component
App.StonehearthCrafterBuildWorkshopView = App.View.extend({
   templateName: 'stonehearthCrafterBuildWorkshop',
   uriProperty: 'context.model',
   intialized: false,

   components: {
      "stonehearth:job" : {
         "job_uri" : {
         }
      },
   },

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      this._super();
      if (!this.initialized) {
         this.createWorkbench();   
         this.initialized = true;
      }
   },

   createWorkbench: function() {
      radiant.call('radiant:play_sound', {'track': 'stonehearth:sounds:ui:start_menu:popup'});
      radiant.call('radiant:play_sound', {'track': 'stonehearth:sounds:ui:start_menu:page_up'});
      var self = this;
      // xxx, localize
      App.stonehearthClient.showTip('Click to place your workshop');

      // kick off a request to the client to show the cursor for placing
      // the workshop. The UI is out of the 'create workshop' process after
      // this. All the work is done in the client and server

      var workbenchEntity = null;
      var workbenchType = this.get('context.model.stonehearth:job.job_uri.workshop.workbench_type');
      var crafter = self.get('context.model');
      radiant.call('stonehearth:choose_workbench_location', workbenchType, crafter = crafter.__self)
         .done(function(o){
            App.stonehearthClient.hideTip();
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
         })
         .always(function() {
            self._finish();
         })
         .fail(function() {
            self._finish();
         });
   },

   _finish : function() {
      App.stonehearthClient.hideTip();
      this.invokeDestroy();      
   },
});