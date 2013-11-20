$(document).ready(function(){

   $(top).on("build_workshop.stonehearth", function (_, e) {
      var view = App.gameView.addView(App.StonehearthCrafterBuildWorkshopView, {
         uri: e.uri
      });
   });
});

// Expects the uri to be an entity with a stonehearth:workshop
// component
App.StonehearthCrafterBuildWorkshopView = App.View.extend({
   templateName: 'stonehearthCrafterBuildWorkshop',
   intialized: false,

   init: function() {
      this._super();

   },

   didInsertElement: function() {
      var self = this;
      var infoUri = this.get('info');
      /*
      $.get(infoUri)
         .done(function(info) {
            console.log(info);
         })
      */

      if (!this.intialized)  {
         $('#crafterBuildWorkshopScroll') //xxx make aop
            .hide()
            .fadeIn();

         //xxx, get this from the profession description on the server
         this.createWorkbench('stonehearth:carpenter:workbench');

         this.intialized = true;
      }
   },

   createWorkbench: function(workbenchType) {
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
      radiant.call('stonehearth:choose_workbench_location', workbenchType)
         .done(function(o){
            workbenchEntity = o.workbench_entity
            self._gotoPage2();
         })
         .always(function(o) {
            $(top).trigger('radiant_hide_tip');
            $(top).trigger('radiant_show_tip', {
               title : "Click and drag to create your crafter's outbox",
               description : 'Your crafter will store crafted goods in the outbox.'
            });

            if (workbenchEntity) {
               radiant.call('stonehearth:choose_outbox_location', workbenchEntity)
                  .done(function(o) {
                     self._finish();
                  })
            }
         });
   },

   _gotoPage2 : function() {
      this._hideScroll('#page1');
   },

   _finish : function() {
      var self = this;

      $(top).trigger('radiant_hide_tip');
      this._hideScroll('#page2', function() {
         self.destroy();
      });
   },

   _hideScroll: function(id, callback) {
     $(id).animate({ 'bottom' : -400 }, 200, function() { callback(); }); 
   },


});