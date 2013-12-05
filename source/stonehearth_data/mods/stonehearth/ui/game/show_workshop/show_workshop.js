$(document).ready(function(){
   $(top).on("radiant_show_workshop", function (_, e) {
      var view = App.gameView.addView(App.StonehearthCrafterView, { uri: e.entity });
   });

   $(top).on("radiant_show_workshop_from_crafter", function (_, e) {
      var view = App.gameView.addView(App.StonehearthCrafterView, { uri: e.event_data.workshop });
   });

});

// Expects the uri to be an entity with a stonehearth:workshop
// component
App.StonehearthCrafterView = App.View.extend({
   templateName: 'stonehearthCrafter',

   components: {
      "unit_info": {},
      "stonehearth:workshop": {
         "crafter": {
            "unit_info" : {},
            "stonehearth:crafter": {
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

   modal: true,

   //alias for stonehearth:workshop.crafter.stonehearth:crafter.craftable_recipes
   recipes: null,

   //alias because the colon messes up bindAttr
   skinClass: function() {
      this.set('context.skinClass', this.get('context.stonehearth:workshop.skin_class'));
   }.observes('context.shonehearth:workshop.skin_class'),

   destroy: function() {
      radiant.keyboard.setFocus(null);
      this._super();
   },

   getWorkshop: function() {
      return this.get('context.stonehearth:workshop').__self;
   },

   getCurrentRecipe: function() {
      return this.get('context.current');
   },

   actions: {
      hide: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:menu_closed' );
         var self = this;
         $("#craftWindow")
            .animate({ top: -1900 }, 500, 'easeOutBounce', function() {
               self.destroy()
         });
      },

      select: function(object, remaining, maintainNumber) {
         this.set('context.current', object);
         this._setRadioButtons(remaining, maintainNumber);
         //TODO: make the selected item visually distinct
         this.preview();
      },

      //Call this function when the user is ready to submit an order
      craft: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:confirm' );
         var workshop = this.getWorkshop();
         var recipe = this.getCurrentRecipe();

         var condition = {};
         var type = $('input[name=conditionGroup]:checked').val();
         if (type == "make") {
            condition.amount = $('#makeNumSelector').val();
         } else {
            condition.inventory_below = $('#mantainNumSelector').val();
         }

         radiant.call_obj(workshop, 'add_order', recipe, condition);
      },

      togglePause: function(){
         var workshop = this.getWorkshop();
         //if (this.get('context.workshopIsPaused') {
            // play the open sound
            //radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:open' );
         //} else {
            // play the closed sound
           // radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:closed' );
        // }

         radiant.call_obj(workshop, 'toggle_pause');
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

      if (this.get('context.stonehearth:workshop') == undefined) {
         return;
      }

      this._buildAccordion();
      this._buildOrderList();
      initIncrementButtons();

      $("#craftWindow")
         .animate({ top: 0 }, {duration: 500, easing: 'easeOutBounce'});

      $("#craftButton").hover(function() {
            $(this).find('#craftButtonLabel').fadeIn();
         }, function () {
            $(this).find('#craftButtonLabel').fadeOut();
         });     
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
      var workshop = this.getWorkshop();
      var recipe = this.getCurrentRecipe();

      radiant.call_obj(workshop, 'resolve_order_options', recipe)
         .done(function(return_data){
            $("#portrait").attr("src", return_data.portrait);
            $("#usefulText").html(return_data.description);
            $("#flavorText").html(return_data.flavor);
         });
   },

   _workshopIsPausedAlias: function() {
      var isPaused = this.get('context.stonehearth:workshop.is_paused');
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

   }.observes('context.stonehearth:workshop.is_paused'),

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
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:menu_open' );
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
      var craftableRecipeArr = this.get('context.stonehearth:workshop.crafter.stonehearth:crafter.craftable_recipes')
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
               var workshop = self.getWorkshop();
               var id = parseInt(ui.item.attr("data-orderid"))
               radiant.call_obj(workshop, 'delete_order', id)
                  .done(function(return_data){
                     ui.item.remove();
                     radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:trash' );
                  });
             }
         },
         over: function (event, ui) {
            //Called whenever we hover over a new target
            if (event.target.id == "garbageList") {
               ui.item.find(".deleteLabel").addClass("showDelete");
               radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:highlight' );
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
            var workshop = self.getWorkshop();
            var newPos = ui.item.index(".orderListItem") + 1;            
            var id =  parseInt(ui.item.attr("data-orderid"));

            //Check if we're replacing?
            if ($(this).attr('data-previndex') == newPos) {
               return;
            }

            radiant.call_obj(workshop, 'move_order', id, newPos);
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
   }.observes('context.stonehearth:workshop.order_list'),

   _enableDisableTrash: function() {
      var list = this.get('context.stonehearth:workshop.order_list');
      if (list && list.length > 0) {
         $('#garbageButton').css('opacity', '1');
      } else {
         $('#garbageButton').css('opacity', '0.3');
      }
   }

});