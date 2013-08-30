$(document).ready(function(){
   i18n.loadNamespace('stonehearth_crafter', function() { console.log('loaded crafter i18n namespace'); });

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
      var workbenchEntity = e.workbench;

      $.get(crafterModUri, { entity: workbenchEntity })
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

   hide: function() {
      var self = this;
      $("#craftingUI")
         .animate({ top: -1900 }, 500, 'easeOutBounce', function() {
            self.destroy();
      });
      $(".overlay")
         .animate({ opacity: 0.0 }, {duration: 300, easing: 'easeInQuad'});
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
      this._initPauseSign();
      this._buildAccordion();
      this._buildOrderList();
      initIncrementButtons();
      this._initializeTooltips();

      $("#craftingUI")
         .animate({ top: 0 }, {duration: 500, easing: 'easeOutBounce'});
      $(".overlay")
         .animate({ opacity: 0.3 }, {duration: 300, easing: 'easeInQuad'});
   },

   //Call this function when the selected order changes.
   select: function(object, remaining, maintainNumber) {
      this.set('context.current', object);
      this._setRadioButtons(remaining, maintainNumber);
      //TODO: make the selected item visually distinct
      this.preview();
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
      $('.tooltip').remove();
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
      var self = this;
      var element = $("#recipeAccordion");
      element.accordion({
         active: 1,
         animate: false,
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
         //$('#orderListUpBtn').show();
         //$('#orderListDownBtn').show();
      });
   },

   _scrollOrderList: function(amount) {
      var orderList = $('#orders'),
      localScrollTop = orderList.scrollTop() + amount;
      orderList.animate({scrollTop: localScrollTop}, 100);
   },

   _initPauseSign: function() {
      this._playPause();
      var $pauseLink = $('#pauseLink');
      $pauseLink.hover(
          function() {
              var $this = $(this); // caching $(this)
              $this.data('initialText', $this.text());
              $this.text(i18n.t('stonehearth_crafter:resume'));
          },
          function() {
              var $this = $(this); // caching $(this)
              $this.text($this.data('initialText'));
          }
      );
      //TODO: is this ok? How did the template get restricted to just context?
      this.set('context.craftingStatus', '');
   },

   _isPausedObserver: function() {
      this._playPause();
   }.observes('context.stonehearth_crafter:workshop.is_paused'),


   _playPause: function() {
      var $pauseBtn = $('#pauseButton');
      if (this.get('context.stonehearth_crafter:workshop.is_paused')) {
         $('#pausedLabel').show();
         this.set('context.craftingStatus', i18n.t('stonehearth_crafter:paused_status'));
         $pauseBtn.removeClass('showPause');
         $pauseBtn.addClass('showPlay');
         $pauseBtn.attr('data-original-title',i18n.t('stonehearth_crafter:pause_tooltip'));
      } else {
         $('#pausedLabel').hide();
         this.set('context.craftingStatus', i18n.t('stonehearth_crafter:crafting_status'));
         $pauseBtn.removeClass('showPlay');
         $pauseBtn.addClass('showPause');
         $pauseBtn.attr('data-original-title',i18n.t('stonehearth_crafter:play_tooltip'));
      }
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
   },

   _initializeTooltips: function() {
      this.set('context.craftRadioTooltip', i18n.t('stonehearth_crafter:craft_radio_tooltip'));
      this.set('context.maintainRadioTooltip', i18n.t('stonehearth_crafter:maintain_radio_tooltip'));

      this.set('context.craftButtonTooltip', i18n.t('stonehearth_crafter:add_order_tooltip'));

      this.set('context.pauseLinkTooltip', i18n.t('stonehearth_crafter:pause_tooltip'));
      this.set('context.garbageButtonTooltip', i18n.t('stonehearth_crafter:delete_tooltip'));

      //$('#garbageButton').attr('data-original-title', i18n.t('stonehearth_crafter:delete_tooltip'));
      //$('#craftButton').attr('data-original-title', i18n.t('stonehearth_crafter:add_order_tooltip'));

      $('#craftOrderOption').tooltip();
      $('#maintainOrderOption').tooltip();

      $('#pauseLink').tooltip();
      $('#garbageButton').tooltip();
      $('#pauseButton').tooltip();
      $('#craftButton').tooltip();
   }


});