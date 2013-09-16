$(document).ready(function(){

   $(top).on("build_workshop.stonehearth_crafter", function (_, e) {
      var view = App.gameView.addView(App.StonehearthCrafterBuildWorkshopView, {
         uri: e.uri
      });
   });

   // handle requests from elsewhere in the UI that a workshop be created
   $(top).on("create_workshop.radiant", function (_, e) {
      // xxx, localize
      $(top).trigger('show_tip.radiant', {
         title : 'Place your workshop',
         description : 'The carpenter uses his workshop to build stuff!',
         cssClass : 'blueprint',
         css : {
            top: 400,
            right: 20
         }
      });

      // kick off a request to the client to show the cursor for placing
      // the workshop. The UI is out of the 'create workshop' process after
      // this. All the work is done in the client and server

      var crafterModUri = '/modules/client/stonehearth_crafter/create_workbench';
      var workbench_entity = e.workbench_entity;

      $.get(crafterModUri, { workbench_entity: workbench_entity })
         .done(function(o){
            //xxx, place the outbox
         })
         .always(function(o) {
            $(top).trigger('hide_tip.radiant');
         });
   });

});

// Expects the uri to be an entity with a stonehearth_crafter:workshop
// component
App.StonehearthCrafterBuildWorkshopView = App.View.extend({
   templateName: 'stonehearthCrafterBuildWorkshop',
   modal: true,

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

      
      if (this.get('context')) {
         $('#crafterBuildWorkshopScroll') //xxx make aop
            .hide()
            .fadeIn();

         self._showApproveButton();
      }

   },

   actions: {
      approve: function() {

         var self = this;
         $('#approveButton')
            // animate down
            .animate({ top: 520 }, 100 , function() {
               //animate up
               $('#approvedStamp').show();
               $(this).delay(200).animate({ top: 400 }, 150, function () {
                  //build the workshop and hide the scroll

                  setTimeout(function() {
                        $(top).trigger('create_workshop.radiant', {
                           workbench_entity: 'entity(stonehearth_carpenter_class, carpenter_workbench)'
                        });
                        self.destroy();
                     }, 1000);
               });
            });
            

         /*
         var person = this.get('context.citizenToPromote');

         var data = {
            targetPerson: person.__self,
            data: this.promoteParams
         };

         var self = this;
         $.ajax({
            type: 'post',
            url: this.promoteUri,
            contentType: 'application/json',
            data: JSON.stringify(data)
         }).done(function(return_data){
            self.destroy();
         });
         */
      }
   },

   _showApproveButton: function() {
      $('#approveButton')
         .show()
         .pulse();

      /*
      var self = this;
      var picker = App.gameView.addView(App.StonehearthPeoplePickerView, {
         uri: '/server/objects/stonehearth_census/worker_tracker',
         title: 'Choose the worker to promote', //xxx localize
         css: {
            top: 350,
            left: 105
         },
         callback: function(person) {
            self.set('context.citizenToPromote', person);

            $('#promoteButton')
               .show()
               .pulse()
         }
      });
      */
   }

});