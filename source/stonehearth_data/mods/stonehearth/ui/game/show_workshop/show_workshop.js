$(document).ready(function(){
   App.stonehearth.showWorkshopView = null;

   $(top).on("radiant_show_workshop", function (_, e) {
      if (App.stonehearth.showWorkshopView) {
         App.stonehearth.showWorkshopView.hide();
      } else {
         App.stonehearth.showWorkshopView = App.gameView.addView(App.StonehearthCrafterView, { uri: e.entity });         
      }
   });

   $(top).on("radiant_show_workshop_from_crafter", function (_, e) {
      if (App.stonehearth.showWorkshopView) {
         App.stonehearth.showWorkshopView.hide();
      } else {
         App.stonehearth.showWorkshopView = App.gameView.addView(App.StonehearthCrafterView, { uri: e.event_data.workshop });
      }
   });

});

// Expects the uri to be an entity with a stonehearth:workshop
// component
App.StonehearthCrafterView = App.View.extend({
   templateName: 'stonehearthCrafter',
   uriProperty: 'context.data',
   components: {
      "unit_info": {},
      "stonehearth:workshop": {
         "crafter": {
            "unit_info" : {},
            'stonehearth:job' : {
               'curr_job_info' : {},
               'curr_job_controller' : {}
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

   initialized: false,
   currentRecipe: null,
   isPaused: false,
   curr_class: null,

   //alias because the colon messes up bindAttr
   skinClass: function() {
      this.set('context.skinClass', this.get('context.data.stonehearth:workshop.skin_class'));
   }.observes('context.data.stonehearth:workshop.skin_class'),

   destroy: function() {
      radiant.keyboard.setFocus(null);
      App.stonehearth.showWorkshopView = null;
      this._super();
   },

   getWorkshop: function() {
      return this.get('context.data.stonehearth:workshop').__self;
   },

   getCurrentRecipe: function() {
      return this.currentRecipe;
   },

   hide: function() {
      var sound = this.get('context.data.stonehearth:workshop.close_sound');

      if (sound) {
         radiant.call('radiant:play_sound', {'track' : sound} );   
      }

      var self = this;
      /* the animation is causing races between when the rest of the UI thinks the view should be destroyed and when it is actually destroyed (at the end of the animation)
      self.$("#craftWindow")
         .animate({ top: -1900 }, 500, 'easeOutBounce', function() {
            self.destroy();
            App.stonehearth.showWorkshopView = null;
      });   
      */

      self.destroy();
   },

   _buildRecipeArray: function() {
      var self = this;
      var recipes = this.get('context.data.stonehearth:workshop.crafter.stonehearth:job.curr_job_info.recipe_list');
      var recipe_categories = [];

      radiant.each(recipes, function(_, category) {
         var recipe_array = [];
         radiant.each(category.recipes, function(recipe_name, recipe_info) {
            var recipe = recipe_info.recipe
            var formatted_recipe = radiant.shallow_copy(recipe);

            //Add ingredient images to the recipes
            formatted_recipe.ingredients = []
            radiant.each(recipe.ingredients, function(i, ingredient) {
               var formatted_ingredient = radiant.shallow_copy(ingredient);
               if (formatted_ingredient.material) {
                  var formatting = App.constants.formatting.resources[ingredient.material];
                  if (formatting) {                     
                     formatted_ingredient.name = formatting.name;
                     formatted_ingredient.icon = formatting.icon;
                  } else {
                     // XXX, roll back to some generic icon
                     formatted_ingredient.name = ingredient.material;
                  }
               } else {
                  radiant.trace(ingredient.uri)
                                    .progress(function(json) {
                                       formatted_ingredient.icon = json.components.unit_info.icon;
                                       formatted_ingredient.name = json.components.unit_info.name;
                                    });
               }
               formatted_recipe.ingredients.push(formatted_ingredient);
            });
            recipe_array.push(formatted_recipe);
         });
         
         //For each of the recipes inside each category, sort them by their level_requirement
         recipe_array.sort(self._compareByLevelAndAlphabetical);

         if (recipe_array.length > 0) {
            var ui_category = {
               category: category.name,
               ordinal:  category.ordinal,
               recipes:  recipe_array,
            };
            recipe_categories.push(ui_category)
         }
      });

      //Sort the recipe categories by ordinal
      recipe_categories.sort(this._compareByOrdinal);

      self.set('recipes', recipe_categories);
   }.observes('context.data.stonehearth:workshop.crafter.stonehearth:job.curr_job_info.recipe_list'),

   //Something with an ordinal of 1 should have precedence
   _compareByOrdinal: function(a, b) {
      return (a.ordinal - b.ordinal);
   },

   //Sort the recipies first by their level requirement, then by their user visible name
   //Note: may have difficulty if we ever get more than 9 crafter levels, but till then this is good.
   _compareByLevelAndAlphabetical: function(a, b) {
      var aName = a.level_requirement + a.recipe_name;
      var bName = b.level_requirement + b.recipe_name;
      if (aName < bName) {
         return -1;
      }
      if (aName > bName) {
         return 1;
      }
      return 0;
   },

   //Updates the recipe display
   _updateRecipesOnLevel: function() {
      Ember.run.scheduleOnce('afterRender', this, '_updateRecipesNow');
   }.observes('context.data.stonehearth:workshop.crafter.stonehearth:job.curr_job_controller'),

   //Show the recipes that should now be visible
   //When this view is initialized, it should remember what type of crafter it's associated with
   //don't update if that's not the current class
   //TODO: test with a dude who has multiple crafter classes (eep)
   _updateRecipesNow: function() {
      var curr_job_controller_data = this.get('context.data.stonehearth:workshop.crafter.stonehearth:job.curr_job_controller');
      if (this.curr_class == null) {
         this.curr_class = curr_job_controller_data.job_name;
      } else if (this.curr_class != curr_job_controller_data.job_name) {
         //If this update does not pertain to the class that this workshop is for, then just return.
         return;
      }

      //All the recipes start hidden
      //If a recipe has a level requirement lower than or equal to the
      //current level in this class show it.
      var curr_level = curr_job_controller_data.last_gained_lv;
      for (var i = 0; i <= curr_level; i++) {
         $("#recipeItems").find("[unlock_level='" + i +"']").css('-webkit-filter', 'grayscale(0%)');
      }
   },

   _setTooltips: function() {
      this.$('[title]').tooltipster();
   },

   actions: {
      hide: function() {
         this.destroy();
      },

      select: function(object, remaining, maintainNumber) {
         this.set('currentRecipe', object);
         Ember.run.scheduleOnce('afterRender', this, '_setTooltips');
         if (this.currentRecipe) {
            //You'd think that when the object updated, the variable would update, but noooooo
            this.set('context.data.current', this.currentRecipe);
            this._setRadioButtons(remaining, maintainNumber);
            //TODO: make the selected item visually distinct
            this.preview();
         }
      },

      //Call this function when the user is ready to submit an order
      craft: function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:carpenter_menu:confirm'} );
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

         if (this.get('context.data.stonehearth:workshop.is_paused')) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:carpenter_menu:open'} );
         } else {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:carpenter_menu:closed'} );
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

   _playOpenSound: function() {
      var sound = this.get('context.data.stonehearth:workshop.open_sound');

      if (sound) {
         radiant.call('radiant:play_sound', {'track' : sound} );   
      }
   }.observes('context.data.stonehearth:workshop.open_sound'),

   // Fires whenever the workshop changes, but the first update is all we really
   // care about.
   _contentChanged: function() {
      Ember.run.scheduleOnce('afterRender', this, '_build_workshop_ui');
    }.observes('recipes'),

    
    _orderCompleted:function() {
      if (this.currentRecipe) {

         //Arrr!! If you try to assign the context at selection, it won't update
         //when the data updates. So when the data updates, check if we should update the context
         //If anyone knows how to do this better with Ember's actual data binding, kill this code!
         var catArr = this.get('context.data.stonehearth:workshop.crafter.stonehearth:job.curr_job_info.recipe_list');
         var catLen = catArr.length;
         for (var i = 0; i < catLen; i++) {
            var recipeArr = catArr[i].recipes;
            var recipeLen = recipeArr.length;
            for (var j=0; j<recipeLen; j++) {
               if (recipeArr[j].recipe_name == this.currentRecipe.recipe_name) {
                  this.set('currentRecipe', recipeArr[j]);
                  break;
               }
            }
         }
         
         this.set('context.data.current', this.currentRecipe);
         this.preview();
      }
    }.observes('context.data.stonehearth:workshop.order_list'),

   //Called once when the model is loaded
   _build_workshop_ui: function() {
      var self = this;

      if (this.get('context.data.stonehearth:workshop') == undefined) {
         return;
      }

      self._buildOrderList();

      self.$("#craftWindow")
         .animate({ top: 0 }, {duration: 500, easing: 'easeOutBounce'});

      self.$("#craftButton").hover(function() {
            $(this).find('#craftButtonLabel').fadeIn();
         }, function () {
            $(this).find('#craftButtonLabel').fadeOut();
         });

      self.$('#searchInput').keyup(function (e) {
         var search = $(this).val();

         if (!search || search == '') {
            self.$('.item').show();
            self.$('.category').show();
         } else {
            // hide items that don't match the search
            self.$('.item').each(function(i, item) {
               var el = $(item);
               var itemName = el.attr('title').toLowerCase();

               if(itemName.indexOf(search) > -1) {
                  el.show();
               } else {
                  el.hide();
               }
            })

            self.$('.category').each(function(i, category) {
               var el = $(category)

               if (el.find('.item:visible').length > 0) {
                  el.show();
               } else {
                  el.hide();
               }
            })
         }
         
      });

      self.$('[title]').tooltipster();
      // select the first recipe
      this.$("#recipeItems").find("[unlock_level='0']")[0].click();

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

      if (workshop && recipe) {
         self.$("#portrait").attr("src", recipe.portrait);
         self.$("#usefulText").html(recipe.description);
         self.$("#flavorText").html(recipe.flavor);

         //stats
         var recipeEntityData = recipe.product_info.entity_data;
         var statHtml = '';
         var statClass = '';
         
         if (recipeEntityData && recipeEntityData['stonehearth:combat:weapon_data']) {
            var damage = recipeEntityData['stonehearth:combat:weapon_data']['base_damage']
            if (damage) {
               statClass = 'damage';
               statHtml = '<div>' + damage + '<br><span class=name>ATK</span></div>';
            }
         } else if (recipeEntityData && recipeEntityData['stonehearth:combat:armor_data']) {
            var armor = recipeEntityData['stonehearth:combat:armor_data']['base_damage_reduction']
            if (armor) {
               statClass = 'armor';
               statHtml = '<div>' + armor + '<br><span class=name>DEF</span></div>';
            }
         }

         self.$("#stats")
            .removeClass()
            .addClass(statClass)
            .html(statHtml);

         var curr_job_controller_data = this.get('context.data.stonehearth:workshop.crafter.stonehearth:job.curr_job_controller');
         var curr_level = curr_job_controller_data.last_gained_lv;
         if (recipe.level_requirement > curr_level) {
            self.$("#craftWindow #orderOptions").hide();
            self.$('#craftWindow #orderOptionsLocked').show();
            self.$('#craftWindow #orderOptionsLocked').text("Unlocked at Level: " + recipe.level_requirement);
         } else {
            self.$("#craftWindow #orderOptions").show();
            self.$('#craftWindow #orderOptionsLocked').hide();            
         }
      }
   },

   _workshopPausedChange: function() {
      var isPaused = !!(this.get('context.data.stonehearth:workshop.is_paused'));

      // We need to check this because if/when the root object changes, all children are 
      // marked as changed--even if the values don't differ.
      if (isPaused == this.isPaused) {
         return;
      }
      this.isPaused = isPaused;

      this.set('context.data.workshopIsPaused', isPaused)

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

   }.observes('context.data.stonehearth:workshop.is_paused'),


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
                     radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:carpenter_menu:trash'} );
                  });
             }
         },
         over: function (event, ui) {
            //Called whenever we hover over a new target
            if (event.target.id == "garbageList") {
               ui.item.find(".deleteLabel").addClass("showDelete");
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:carpenter_menu:highlight'} );
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
   }.observes('context.data.stonehearth:workshop.order_list.orders'),

   _enableDisableTrash: function() {
      var list = this.get('context.data.stonehearth:workshop.order_list.orders');

      if (this.$('#garbageButton')) {
         if (list && list.length > 0) {
            this.$('#garbageButton').css('opacity', '1');
         } else {
            this.$('#garbageButton').css('opacity', '0.3');
         }
      }
   }

});