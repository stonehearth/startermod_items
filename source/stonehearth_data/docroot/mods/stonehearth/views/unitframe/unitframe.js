radiant.views['stonehearth.views.unitframe'] = radiant.View.extend({
   options: {
      position: {
         my: "center bottom",
         at: "center bottom"
      }
   },

   NUM_ACTION_BUTTONS: 5,

   onAdd: function (view) {
      this._super(view);

      this.initActionButtons();
      this.initHandlers();
      //this.clear();
   },

   initActionButtons: function () {
      for (var i = 0; i < this.NUM_ACTION_BUTTONS; i++) {
         var buttonLabel = radiant.keyboard.getKeybind("actionbar_button" + i);

         this.my("#ab" + i).actionbutton({ label: buttonLabel })
            .click(function () {
               var button = $(this).data("actionbutton");
               radiant.api.cmd('execute_action', {
                  'action': button.execute,
                  'self': button.entityId,
               });
            });
      }
   },

   initHandlers: function () {
      var self = this;

      // Listen for selection changed events
      $(top).on("radiant.events.selection_changed", function (_, evt) {
         var entityId = evt.data.entity_id;
         if (self.trace) {
            self.trace.destroy();
         }
         console.log("selection changed detected in unitframe");
         if (entityId == 0) {
            self.clear();
         } else {
            self.trace = radiant.trace_entity(entityId)
                            .get('identity')
                            .get('action_list')
                            .progress(function (data) {
                               self.refresh(entityId, data);
                            });
         }
      });
   },

   clear: function () {
      this.my("#portrait").css("background-image", "");
      this.my("#unitframe #name").html("");
      this.my("#unitframe #description").html("");

      this.my("#actionbar a").hide();
   },

   refresh: function (entityId, data) {
      this.clear();

      this.render("#name", data.identity.name);
      this.render("#description", data.identity.description);

      if (data.action_list) {
         var actions = data.action_list.actions;

         for (var i = 0; i < actions.length; i++) {
            var buttonData = actions[i];
            var abElement = this.my("#ab" + i);
            //abElement.attr('title', buttonData.tooltip);

            var actionbutton = abElement.data("actionbutton");  //get the actionbutton interface
            actionbutton.entityId = entityId;
            actionbutton.icon(buttonData.icon);
            actionbutton.execute = buttonData.action_uri;
            this.my("#ab" + i).show()
         }
      }

      //this._myElement.show();
   }
});
