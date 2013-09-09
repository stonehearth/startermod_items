$(document).ready(function(){
   // When we get the show_workshop event, toggle the crafting window
   // for this entity.
   $(top).on("show_workshop.stonehearth_crafter", function (_, e) {
      //TODO: hide the workshop on X button, etc.
      var view = App.gameView.addView(App.StonehearthCrafterView, { uri: e.entity });
   });

   // handle requests from elsewhere in the UI that a workshop be created
   $(top).on("create_workshop.radiant", function (_, e) {
      // xxx, localize
      $(top).trigger('show_tip.radiant', {
         title : 'Place your workshop',
         description : 'The carpenter uses his workshop to build stuff!'
      });

      // kick off a request to the client to show the cursor for placing
      // the workshop. The UI is out of the 'create workshop' process after
      // this. All the work is done in the client and server

      var crafterModUri = '/modules/client/stonehearth_crafter/create_workbench';
      var workbench_uri = e.workbench_uri;

      $.get(crafterModUri, { workbench_uri: workbench_uri })
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
App.StonehearthCrafterView = App.View.extend({
   templateName: 'stonehearthCrafter',

   components: {
      "unit_info": {},
      "stonehearth_crafter:workshop": {
         "crafter": {
            "stonehearth_crafter:crafter": {
               "craftable_recipes" : {
                  "recipes" : []
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

   init: function() {
      this._super();
   },

   destroy: function() {
      radiant.keyboard.setFocus(null);
      this._super();
   },

   actions: {
      hide: function() {
         var self = this;
         $("#craftingUI")
            .animate({ top: -1900 }, 500, 'easeOutBounce', function() {
               self.destroy();
         });
         $(".overlay")
            .animate({ opacity: 0.0 }, {duration: 300, easing: 'easeInQuad'});
      },

      select: function(object, remaining, maintainNumber) {
         this.set('context.current', object);
         this._setRadioButtons(remaining, maintainNumber);
         //TODO: make the selected item visually distinct
         this.preview();
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
      }
   },

   //Fired when the template has loaded. Continue to
   //run until all relevant children are inserted.
   //Then attach all jquery functionality to items
   didInsertElement: function() {
      this._super();

      if (this.get('context.stonehearth_crafter:workshop') == undefined) {
         return;
      }

      radiant.keyboard.setFocus($("#craftingUI")); // xxx change this to a higher level api, like a 'modal' property on the view

      this._buildAccordion();
      this._buildOrderList();
      initIncrementButtons();

      $("#craftingUI")
         .animate({ top: 0 }, {duration: 500, easing: 'easeOutBounce'});
      $(".overlay")
         .animate({ opacity: 0.3 }, {duration: 300, easing: 'easeInQuad'});
   },

   _setRadioButtons: function(remaining, maintainNumber) {
      //Set the radio buttons correctly
      if (remaining) {
         $("#makeNumSelector").val(remaining);
         $("#make").prop("checked", "checked");
      } else {
         $("#makeNumSelector").val("1");
         $("#make").prop("checked", false);
      }
      if (maintainNumber) {
         $("#mantainNumSelector").val(maintainNumber);
         $("#maintain").prop("checked", "checked");
      } else {
         $("#mantainNumSelector").val("1");
         $("#maintain").prop("checked", false);
      }
      if (!remaining && !maintainNumber) {
         $("#make").prop("checked", "checked");
      }
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

   workshopIsPaused: Ember.computed.alias("stonehearth_crafter:workshop.is_paused"),

   _workshopIsPausedAlias: function() {
      var isPaused = this.get('context.stonehearth_crafter:workshop.is_paused');
      this.set('context.workshopIsPaused', isPaused)

      var r = isPaused ? 4 : -4;

      // flip the sign
      $("#statusSign").animate({  
         rot: r,
         }, 
         {
            duration: 200,
            step: function(now,fx) {
               var percentDone;
               var end = fx.end;
               var start = fx.start;

               if (end > start) {
                  console.log('end > start');
                  percentDone = (now - start) / (end - start);
               } else {
                  percentDone = -1 * (now - start) / (start - end);
               }

               var scaleX = percentDone < .5 ? 1 - (percentDone * 2) : (percentDone * 2) - 1;

               console.log('step = ' + now + ", " + scaleX);

               $(this).css('-webkit-transform', 'rotate(' + now + 'deg) scale(' + scaleX +', 1)');
            }
      });


   }.observes('context.stonehearth_crafter:workshop.is_paused'),

   //Attach accordion functionality to the appropriate div
   _buildAccordion: function() {
      var self = this;
      var element = $("#recipeAccordion");
      element.accordion({
         active: 1,
         animate: true,
         heightStyle: "fill"
      });

      this._buildRecipeArray();

      $( "#searchInput" ).autocomplete({
         source: allRecipes,
         select: function( event, ui ) {
            event.preventDefault();
            //TODO: put the values in a hash by their names
            //TODO: associate functionality with search box
            //TODO: fix search box
            console.log("selecting... " + ui.item.value);
            //$("#searchInput").val(ui.item.label);
            this.val = ui.item.label;
            self.select(ui.item.value)
         },
         focus: function (event, ui) {
            event.preventDefault();
            this.value = ui.item.label;
         },
      }).keydown(function(e){
         //On enter
         var userInput = this.value.toLowerCase();
         if (e.keyCode === 13) {
            self.findAndSelectRecipe();
         }
      });
      // open the first category
      element.find("h3")[0].click();
      element.find("a")[0].click();
   },

   findAndSelectRecipe: function() {
      var userInput = $("#searchInput").val().toLowerCase(),
          currRecipe = this.get('context.current.recipe_name');
      if ( !currRecipe || (currRecipe && (currRecipe.toLowerCase() != userInput)) ) {
         //Look to see if we have a recipe named similar to the contents
         var numRecipes = allRecipes.length;
         for (var i=0; i<numRecipes; i++) {
            if (userInput == allRecipes[i].label.toLowerCase()) {
               this.select(allRecipes[i].value);
               $(".ui-autocomplete").hide();
               break;
            }
         }
      } else if (currRecipe && (currRecipe.toLowerCase() == userInput)) {
         //If the recipe is already selected and the menu is open, just hide the menu
         $(".ui-autocomplete").hide();
      }
   },

   allRecipes: null,

   _buildRecipeArray: function() {
      allRecipes = new Array();
      var craftableRecipeArr = this.get('context.stonehearth_crafter:workshop.crafter.stonehearth_crafter:crafter.craftable_recipes')
      var numCategories = craftableRecipeArr.length;
      for (var i = 0; i < numCategories; i++) {
         var recipes = craftableRecipeArr[i].recipes;
         var numRecipes = recipes.length;
         for (var j = 0; j < numRecipes; j++) {
            allRecipes.push({
               label: recipes[j].recipe_name ,
               category: craftableRecipeArr[i].category,
               value: recipes[j]});
         }
      }
   },

   //Attach sortable/draggable functionality to the order
   //list. Hook order list onto garbage can. Set up scroll
   //buttons.
   _buildOrderList: function(){
      var self = this;
      $( "#orders, #garbageList" ).sortable({
         axis: "y",
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
      this._enableDisableTrash();
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
   },

   _orderListObserver: function() {
      this._enableDisableTrash();
   }.observes('context.stonehearth_crafter:workshop.order_list'),

   _enableDisableTrash: function() {
      var list = this.get('context.stonehearth_crafter:workshop.order_list');
      if (list && list.length > 0) {
         $('#garbageButton').css('opacity', '1');
      } else {
         $('#garbageButton').css('opacity', '0.3');
      }
   }

});