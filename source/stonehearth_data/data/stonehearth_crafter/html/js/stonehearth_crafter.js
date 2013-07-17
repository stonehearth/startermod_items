$(document).ready(function(){

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
               "craftable_recipes" : {
                  "preview_item" : {}
               }
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
            console.log('!!!!!!!!!!!!!!!!!!!!!!');
            self.destroy();
      });
   },

   didInsertElement: function() {
     this._super();
     this._buildAccordion();
     $("#craftingWindow")
      .delay(200)
      .animate({ top: 20 }, {duration: 500, easing: 'easeOutBounce'});

     //this._buildOrderList();
   },

   //Call this function when the selected order changes.
   select: function(object) {
      this.set('context.current', object);
   },

   //Call this function when the user is ready to submit an order
   craft: function() {
      console.log('craft!');
      //TODO: replace with call to the server
      $("#orders").append('<div class="orderListItem"><a href=#>new</a></div>');
   },

   scrollOrderListUp: function() {
      this._scrollOrderList(-153)
   },

   scrollOrderListDown: function() {
      this._scrollOrderList(153)
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
      //element.find("h3").find("a")[0].click(); // <-- ugly!
   },

   //Attach sortable/draggable functionality to the order
   //list. Hook order list onto garbage can. Set up scroll
   //buttons.
   _buildOrderList: function(){
      $( "#orders, #garbageList" ).sortable({
         axis: "y",
         connectWith: "#garbageList",
         beforeStop: function (event, ui) {
             if(ui.item[0].parentNode.id == "garbageList") {
               ui.item.remove();
             }
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
      });
   },

   _scrollOrderList: function(amount) {
      var orderList = $('#orders'),
      localScrollTop = orderList.scrollTop() + amount;
      orderList.animate({scrollTop: localScrollTop}, 100);
   }


});