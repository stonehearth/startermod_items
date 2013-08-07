$(document).ready(function(){
   i18n.loadNamespace('stonehearth_crafter', function() { console.log('loaded crafter i18n namespace'); });

   // When we get the show_workshop event, toggle the crafting window
   // for this entity.
   $(top).on("stonehearth_crafter.show_workshop", function (_, e) {
      var entity = e.entity;
      var existingWorkshopView = stonehearth.entityData(entity, "show_workshop");

      if (!existingWorkshopView) {
         var view = App.gameView.addView(App.StonehearthCrafterView, { uri: entity });
         stonehearth.entityData(entity, "show_workshop", view);
         console.log('showing crafting ui ' + view + ' for entity ' + entity)
      } else {
         console.log('hiding crafting ui for entity ' + entity)
         existingWorkshopView.hide();
         stonehearth.entityData(entity, "show_workshop", null);
      }
   });

   // when the selection changes, restore the workshop if need be.
   $(top).on("radiant.events.selection_changed", function (_, e) {
      var entity = e.entity;

      // refresh the workshop widget if it us up and we've selected a
      // different workshop. Otherwise, hide the workshop
      if (e["stonehearth_crafter:workshop"] != undefined) {

      } else {
         var existingWorkshopView = stonehearth.entityData(entity, "show_workshop");
         if (existingWorkshopView) {
            existingWorkshopView.hide();
         }
      }

   });

});

// Expects the uri to be an entity with a stonehearth_crafter:workshop
// component
App.StonehearthCrafterView = App.View.extend({
   templateName: 'stonehearthCrafter',

   components: {
      "unit_info": {},
      "stonehearth_crafter:workshop": {
         "crafter": {
            "stonehearth_crafter:crafter": {
               "craftable_recipes" : []
            }
         },
         "order_list" : {
            "recipe" : {}
         }
      }

   },

   //alias for stonehearth_crafter:workshop.crafter.stonehearth_crafter:crafter.craftable_recipes
   recipes: null,

   hide: function() {
      var self = this;
      $("#craftingWindow")
         .animate({ top: -1900 }, 500, 'easeOutBounce', function() {
            self.destroy();
      });
   },

   _hasLoaded: false,

   //Fired when the template has loaded. Continue to
   //run until all relevant children are inserted.
   //Then attach all jquery functionality to items
   didInsertElement: function() {
      if (this.get('context.stonehearth_crafter:workshop') == undefined) {
         return;
      }

      console.log("DidInsertElement is being called on the crafting window");
      console.log(this.get("context.stonehearth_crafter:workshop.order_list"));
      this._super();
      this._buildAccordion();
      this._buildOrderList();
      initIncrementButtons();

      $("#craftingWindow")
         .animate({ top: 20 }, {duration: 500, easing: 'easeOutBounce'});
   },

   //Call this function when the selected order changes.
   select: function(object) {
      this.set('context.current', object);
      //TODO: make the selected item visually distinct
      this.preview();
   },

   preview: function() {
      var url = this.get('context.stonehearth_crafter:workshop').__self + "?fn=resolve_order_options";
      var data = {};
      data.recipe_url = this.get('context.current').my_location;
      $.ajax({
            type: 'post',
            url: url,
            contentType: 'application/json',
            data: JSON.stringify(data)
         }).done(function(return_data){
            $("#portrait").attr("src", return_data.portrait);
            $("#usefulText").html(return_data.desc);
            $("#flavorText").html(return_data.flavor);
         });
   },

   //Call this function when the user is ready to submit an order
   craft: function() {
      var url = this.get('context.stonehearth_crafter:workshop').__self + "?fn=add_order";
      var data = {};
      data.recipe_url = this.get('context.current').my_location;
      var type = $('input[name=conditionGroup]:checked').val();
      if (type == "make") {
         data.condition_amount = $('#makeNumSelector').val();
      } else {
         data.condition_inventory_below = $('#mantainNumSelector').val();
      }
      $.ajax({
            type: 'post',
            url: url,
            contentType: 'application/json',
            data: JSON.stringify(data)
         }).done(function(return_data){
            //TODO: maybe stuff goes here?
         });
   },

   togglePause: function(){
      var url = this.get('context.stonehearth_crafter:workshop').__self + "?fn=toggle_pause";
      var data = {};
      $.ajax({
            type: 'post',
            url: url,
            contentType: 'application/json',
            data: JSON.stringify(data)
         }).done(function(return_data){
            //TODO: change other things to reflect pause?
         });

   },

   scrollOrderListUp: function() {
      this._scrollOrderList(-157)
   },

   scrollOrderListDown: function() {
      this._scrollOrderList(157)
   },

   //Attach accordion functionality to the appropriate div
   _buildAccordion: function() {
      var element = $("#recipeAccordion");
      element.accordion({
         active: 1,
         animate: false,
         heightStyle: "fill"
      });

      // open the first category
      element.find("h3")[0].click();
      element.find("a")[0].click();
      //element.find("h3").find("a")[0].click(); // <-- ugly!
   },

   //Attach sortable/draggable functionality to the order
   //list. Hook order list onto garbage can. Set up scroll
   //buttons.
   _buildOrderList: function(){
      var self = this;
      $( "#orders, #garbageList" ).sortable({
         connectWith: "#garbageList",
         beforeStop: function (event, ui) {
            //Called right after an object is dropped
            if(ui.item[0].parentNode.id == "garbageList") {
               ui.item.addClass("hiddenOrder");
               var url = self.get('context.stonehearth_crafter:workshop').__self + "?fn=delete_order";
               var data = {
                  id: parseInt(ui.item.attr("data-orderid"))
               };
               $.ajax({
                  type: 'post',
                  url: url,
                  contentType: 'application/json',
                  data: JSON.stringify(data)
               }).done(function(return_data){
                  ui.item.remove();
               });
             }
         },
         over: function (event, ui) {
            //Called whenever we hover over a new target
            if (event.target.id == "garbageList") {
               ui.item.find(".deleteLabel").addClass("showDelete");
            } else {
               ui.item.find(".deleteLabel").removeClass("showDelete");
            }
         },
         start: function(event, ui) {
            // on drag start, creates a temporary attribute on the element with the old index
            $(this).attr('data-previndex', ui.item.index(".orderListItem")+1);
         },
         update: function (event, ui) {
            //Called right when we're sorting
            //Don't update objects inside the garbage list
            if(ui.item[0].parentNode.id == "garbageList") {
               return;
            }
            var url = self.get('context.stonehearth_crafter:workshop').__self + "?fn=move_order";
            var data = {
               newPos: (ui.item.index(".orderListItem") + 1),
               id: parseInt(ui.item.attr("data-orderid"))
            };
            //Check if we're replacing?
            if ($(this).attr('data-previndex') == data.newPos) {
               return;
            }
            $.ajax({
               type: 'post',
               url: url,
               contentType: 'application/json',
               data: JSON.stringify(data)
            }).done(function(return_data){
            });

         }
      }).disableSelection();
      this._initButtonStates();
   },

   //Initialize button states to visible/not based on contents of
   //list. Register an event to toggle them when they
   //are no longer needed. TODO: animation!
   _initButtonStates: function() {
      var currentOrdersList = $('#orders');
      //Set the default state of the buttons
      if (currentOrdersList[0].scrollHeight > currentOrdersList.height()) {
         $('#orderListUpBtn').show();
         $('#orderListDownBtn').show();
      } else {
         $('#orderListUpBtn').hide();
         $('#orderListDownBtn').hide();
      }

      //Register an event to toggle the buttons when the scroll state changes
      currentOrdersList.on("overflowchanged", function(event){
         console.log("overflowchanged!");
         $('#orderListUpBtn').toggle();
         $('#orderListDownBtn').toggle();
         //$('#orderListUpBtn').show();
         //$('#orderListDownBtn').show();
      });
   },

   _scrollOrderList: function(amount) {
      var orderList = $('#orders'),
      localScrollTop = orderList.scrollTop() + amount;
      orderList.animate({scrollTop: localScrollTop}, 100);
   }


});