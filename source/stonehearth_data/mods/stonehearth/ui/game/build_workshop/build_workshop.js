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
   intialized: false,

      components: {
         "stonehearth:profession" : {
            "profession_uri" : {}
         },
      },
   
   init: function() {
      this._super();

   },

   didInsertElement: function() {
      var self = this;
      if (!this.initialized) {
         this.createWorkbench();   
         this.initialized = true;
      }
   },

   createWorkbench: function() {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:page_up' );
      var self = this;
      // xxx, localize
      $(top).trigger('radiant_show_tip', {
         title : 'Place your workshop',
         description : 'Click in the spot where you want to put this new workshop!'
      });

      // kick off a request to the client to show the cursor for placing
      // the workshop. The UI is out of the 'create workshop' process after
      // this. All the work is done in the client and server

      var workbenchEntity = null;
      var workbenchType = this.get('context.stonehearth:profession.profession_uri.workshop.workbench_type');
      radiant.call('stonehearth:choose_workbench_location', workbenchType)
         .done(function(o){
            workbenchEntity = o.workbench_entity
         })
         .always(function(o) {
            $(top).trigger('radiant_hide_tip');
            
            if (workbenchEntity) {
               self._gotoPage2();

               $(top).trigger('radiant_show_tip', {
                  title : "Click and drag to create your crafter's outbox",
                  description : 'Your crafter will store crafted goods in the outbox.'
               });

               var crafter = self.get('context');
               radiant.call('stonehearth:choose_outbox_location', workbenchEntity, crafter = crafter.__self)
                  .done(function(o) {
                     if (o.cancelled) {
                        $(top).trigger('radiant_hide_tip');
                        self.destroy();
                     } else {
                        self._finish();
                     }
                  })
            } else {
               self.destroy();
            }
         });
   },

   _gotoPage2 : function() {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
      this.$('#page2').show();
   },

   _finish : function() {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
      var self = this;

      $(top).trigger('radiant_hide_tip');
      this.destroy();
   },


});