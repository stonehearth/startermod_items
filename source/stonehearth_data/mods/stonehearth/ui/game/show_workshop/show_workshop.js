$(document).ready(function(){
   $(top).on("radiant_show_workshop", function (_, e) {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:menu_open' );
      var view = App.gameView.addView(App.StonehearthCrafterView, { uri: e.entity });
   });

   $(top).on("radiant_show_workshop_from_crafter", function (_, e) {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:menu_open' );
      var view = App.gameView.addView(App.StonehearthCrafterView, { uri: e.event_data.workshop });
   });

});

// Expects the uri to be an entity with a stonehearth:workshop
// component
App.StonehearthCrafterView = App.View.extend({
   templateName: 'stonehearthCrafter',

   uriProperty: 'context.model',

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
            "orders" : {
               "recipe" : {}
            }
         }
      }

   },

   modal: true,

   //alias for stonehearth:workshop.crafter.stonehearth:crafter.craftable_recipes
   recipes: null,

   initialized: false,

   currentRecipe: null,

   //alias because the colon messes up bindAttr
   skinClass: function() {
      this.set('context.skinClass', this.get('context.model.stonehearth:workshop.skin_class'));
   }.observes('context.model.stonehearth:workshop.skin_class'),

   destroy: function() {
      radiant.keyboard.setFocus(null);
      this._super();
   },

   getWorkshop: function() {
      return this.get('context.model.stonehearth:workshop').__self;
   },

   getCurrentRecipe: function() {
      return this.currentRecipe;
   },

   actions: {
      hide: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:menu_closed' );
         var self = this;
         self.$("#craftWindow")
            .animate({ top: -1900 }, 500, 'easeOutBounce', function() {
               self.destroy()
         });
      },

      select: function(object, remaining, maintainNumber) {
         this.currentRecipe = object;
         this.set('context.model.current', this.currentRecipe);
         this._setRadioButtons(remaining, maintainNumber);
         //TODO: make the selected item visually distinct
         this.preview();
      },

      //Call this function when the user is ready to submit an order
      craft: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:confirm' );
         var workshop = this.getWorkshop();
         var recipe = this.getCurrentRecipe();

         var condition;
         var type = $('input[name=conditionGroup]:checked').val();
         if (type == "make") {
            condition = {
               type: "make",
               amount: this.$('#makeNumSelector').val(),
            }            
         } else if (type == "maintain") {
            condition = {
               type: "maintain",
               at_least: this.$('#maintainNumSelector').val(),
            }            
         }

         radiant.call_obj(workshop, 'add_order', recipe, condition);
      },

      togglePause: function(){
         var workshop = this.getWorkshop();

         if (this.get('context.model.workshopIsPaused')) {
            radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:open' );
         } else {
            radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:closed' );
         }

         radiant.call_obj(workshop, 'toggle_pause');
      },

      scrollOrderListUp: function() {
         this._scrollOrderList(-157)
      },

      scrollOrderListDown: function() {
         this._scrollOrderList(157)
      }
   },

   didInsertElement: function() {
      this._super();
   },

   // Fires whenever the workshop changes, but the first update is all we really
   // care about.
   _contentChanged: function() {
      if (this.get('context.model.stonehearth:workshop') == undefined) {
         return;
      }

      // A new context.model completely clobbers the old one, so don't forget
      // to set the current recipe.  There has to be a better way of doing this....
      if (this.currentRecipe) {
         this.set('context.model.current', this.currentRecipe);
      }

      if (this.initialized) {
         return
      }
      this.initialized = true;
      Ember.run.scheduleOnce('afterRender', this, '_build_workshop_helper');
    }.observes('context.model'),

   //Called once when the model is loaded
   _build_workshop_helper: function() {
      if (this.get('context.model.stonehearth:workshop') == undefined) {
         return;
      }

      this._buildRecipeList();
      this._buildOrderList();
      initIncrementButtons();

      this.$("#craftWindow")
         .animate({ top: 0 }, {duration: 500, easing: 'easeOutBounce'});

      this.$("#craftButton").hover(function() {
            $(this).find('#craftButtonLabel').fadeIn();
         }, function () {
            $(this).find('#craftButtonLabel').fadeOut();
         });
   }, 

   _setRadioButtons: function(remaining, maintainNumber) {
      //Set the radio buttons correctly
      if (remaining) {
         this.$("#makeNumSelector").val(remaining);
         this.$("#make").prop("checked", "checked");
      } else {
         this.$("#makeNumSelector").val("1");
         this.$("#make").prop("checked", false);
      }
      if (maintainNumber) {
         this.$("#maintainNumSelector").val(maintainNumber);
         this.$("#maintain").prop("checked", "checked");
      } else {
         this.$("#maintainNumSelector").val("1");
         this.$("#maintain").prop("checked", false);
      }
      if (!remaining && !maintainNumber) {
         this.$("#make").prop("checked", "checked");
      }
   },

   preview: function() {
      var self = this;
      var workshop = this.getWorkshop();
      var recipe = this.getCurrentRecipe();

      radiant.call_obj(workshop, 'resolve_order_options', recipe)
         .done(function(return_data){
            var portrait = self.$("#portrait");
            if (portrait) {
               // we assuming that if the portrait div is there, everything else
               // is too.  this could go away if the sheet is closed before our
               // call comes back!
               self.$("#portrait").attr("src", return_data.portrait);
               self.$("#usefulText").html(return_data.description);
               self.$("#flavorText").html(return_data.flavor);
            }
         });
   },

   _workshopIsPausedAlias: function() {
      var isPaused = !!(this.get('context.model.stonehearth:workshop.is_paused'));
      this.set('context.model.workshopIsPaused', isPaused)

      var r = isPaused ? 4 : -4;

      // flip the sign
      var sign = this.$("#statusSign");

      if (sign) {
         sign.animate({
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
                  $(this).css('-webkit-transform', 'rotate(' + now + 'deg) scale(' + scaleX +', 1)');
               }
         });
      }

   }.observes('context.model.stonehearth:workshop.is_paused'),

   _buildRecipeList: function() {
      var self = this;

      this._buildRecipeArray();

      this.$( "#searchInput" ).autocomplete({
         source: allRecipes,
         select: function( event, ui ) {
            event.preventDefault();
            //TODO: put the values in a hash by their names
            //TODO: associate functionality with search box
            //TODO: fix search box
            console.log("selecting... " + ui.item.value);
            //$("#searchInput").val(ui.item.label);
            this.val = ui.item.label;
            self.send('select', ui.item.value)
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
      }).focus();
      
      // select the first recipe
      this.$("#recipeItems").find("a")[0].click();
     
   },

   findAndSelectRecipe: function() {
      var userInput = this.$("#searchInput").val().toLowerCase(),
          currRecipe = this.get('context.model.current.recipe_name');
      if ( !currRecipe || (currRecipe && (currRecipe.toLowerCase() != userInput)) ) {
         //Look to see if we have a recipe named similar to the contents
         var numRecipes = allRecipes.length;
         for (var i=0; i<numRecipes; i++) {
            if (userInput == allRecipes[i].label.toLowerCase()) {
               this.send('select', allRecipes[i].value);
               this.$(".ui-autocomplete").hide();
               break;
            }
         }
      } else if (currRecipe && (currRecipe.toLowerCase() == userInput)) {
         //If the recipe is already selected and the menu is open, just hide the menu
         this.$(".ui-autocomplete").hide();
      }
   },

   allRecipes: null,

   _buildRecipeArray: function() {
      allRecipes = new Array();
      var craftableRecipeArr = this.get('context.model.stonehearth:workshop.crafter.stonehearth:crafter.craftable_recipes')
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
      this.$( "#orders, #garbageList" ).sortable({
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
      var self = this;
      if (currentOrdersList[0].scrollHeight > currentOrdersList.height()) {
         self.$('#orderListUpBtn').show();
         self.$('#orderListDownBtn').show();
      } else {
         self.$('#orderListUpBtn').hide();
         self.$('#orderListDownBtn').hide();
      }

      //Register an event to toggle the buttons when the scroll state changes
      currentOrdersList.on("overflowchanged", function(event){
         console.log("overflowchanged!");
         self.$('#orderListUpBtn').toggle();
         self.$('#orderListDownBtn').toggle();
      });
   },

   _scrollOrderList: function(amount) {
      var orderList = this.$('#orders'),
      localScrollTop = orderList.scrollTop() + amount;
      orderList.animate({scrollTop: localScrollTop}, 100);
   },

   _orderListObserver: function() {
      this._enableDisableTrash();
   }.observes('context.model.stonehearth:workshop.order_list.orders'),

   _enableDisableTrash: function() {
      var list = this.get('context.model.stonehearth:workshop.order_list.orders');

      if (this.$('#garbageButton')) {
         if (list && list.length > 0) {
            this.$('#garbageButton').css('opacity', '1');
         } else {
            this.$('#garbageButton').css('opacity', '0.3');
         }
      }
   }

});