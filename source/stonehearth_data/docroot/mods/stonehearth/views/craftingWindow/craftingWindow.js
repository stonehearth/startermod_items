radiant.views['stonehearth.views.craftingWindow'] = radiant.Dialog.extend({
   recipes: null,
   url: "/views/craftingWindow",

   options: {
      dialogOptions: {
         title: "Crafting",
         width: 770,
         height: 860
      },
      position: {
         my: "right - 20",
         at: "right"
      },
      resizable: false,
      hidden: true
   },

   NUM_ACTION_BUTTONS: 5,

   onAdd: function (view) {
      this._super(view);

      this.recipes = null;
      this.currentRecipe = null;
      var self = this;

      // setup ui elements
      //v.parent().addClass("dialog-loading");
      //v.dialog("option", "buttons", { Close: function () { $(this).dialog("close"); } });
      this.my("#craftingWindow").tabs();
      this.my("#sortable").sortable();
      this.my("#sortable").disableSelection();
      this.my("#craftButton").button();
      this.my("#details").scale9Grid({ top: 160, bottom: 160, left: 160, right: 160 });
      this.my("#accordion").accordion({ heightStyle: "fill" });
      this.my("#details-frame").scale9Grid({ top: 40, bottom: 40, left: 40, right: 40 });

      // event handlers
      this.my("#search").focus(function (e) {
         var input = $(this);
         if (input.val() == "Search") {
            input.val("");
         }
      }).keyup(function (e) {
         var input = $(this);
         if (input.val().length > 0) {
            self.search(input.val());
         } else {
            self.refresh();
         }
      });
      this.my("#craftButton").click(function (e) {
         if (self.currentRecipe && self.crafterEntityId) {
            radiant.api.execute('radiant.actions.craft', {
               self: self.crafterEntityId,
               recipe: self.currentRecipe,
               ingredients: {
                  blade: 'oak-log',
                  hilt:  'oak-log',
               }
            });
         }
      });

      this.my("[id^=work-option]").click(function (e) {
         self.updateWorkOptions();
      });

      $(top).on("radiant.events.selection_changed", function (_, evt) {
         self.show(evt.data.entity_id);
      });

   },

   show: function (crafterEntityId) {
      console.log("populating the crafting window for " + crafterEntityId);

      this.crafterEntityId = crafterEntityId;
      if (this.crafterEntityId) {
         var self = this;
         var trace = radiant.trace_entity(this.crafterEntityId)
            .get("profession")
            .progress(function (result) {
               trace.destroy(); // Crashes V8?!?  WTF??? =..(
               if (result.profession && result.profession.learned_recipes) {
                  radiant.api.cmd('fetch_json_data', { entries: result.profession.learned_recipes })
                     .done(function (recipes) {
                        self.recipes = recipes;
                        self.refresh();
                     });
               } else {
                  // probably not the right behavior...
                  self.recipes = {}
                  self.refresh();
               }
            });
      }
   },

   refresh: function () {
      var self = this;
      var accordion = this.my("#accordion");

      accordion.accordion("destroy");
      accordion.html("");

      this.currentRecipe = null;

      // add each recipe to the accordion
      $.each(this.recipes, function (key, recipe) {
         var categories = {};  // table of jquery elements by category name

         $.each(recipe.categories, function (i, category) {
            if (categories[category] == undefined) {
               categories[category] = self.buildCategoryElement(category);
               accordion.append(categories[category]);
            }
            var list = categories[category].find("ul");
            self.addRecipe(key, list);
         });

      });

      accordion.accordion({ heightStyle: "fill" });

      this.setupLinkHandlers();
      //this.updateWorkOptions();
   },

   search: function (search) {
      console.log("searching crafting window on " + search);
      var accordion = this.my("#accordion");
      accordion.accordion("destroy");
      accordion.html("");

      var searchCategory = this.buildCategoryElement("Search");
      accordion.append(searchCategory);

      var list = searchCategory.find("ul");

      $.each(this.recipes, function (key, recipe) {
         if (recipe.name.toLowerCase().indexOf(search.toLowerCase()) !== -1) {
            self.addRecipe(key, list);
         }
      });

      accordion.accordion({ heightStyle: "fill" });
      this.setupLinkHandlers();
   },


   buildCategoryElement: function (name) {
      return $("<h3>" + name + "</h3><div><ul class='simple-list'></ul></div>");
   },

   addRecipe: function (key, ul) {
      ul.append("<li><a class='recipe-link' recipeKey='" + key + "' href='#'>" + this.recipes[key].name + "</a></li>");

      if (!this.currentRecipe) {
         this.selectRecipe(key);
      }
   },

   selectRecipe: function (key) {
      this.refreshScroll(key);
      this.currentRecipe = key;
   },

   refreshScroll: function (key) {
      var self = this;
      var recipe = this.recipes[key];
      this.render("#recipeName", recipe.name);
      this.render("#recipeDescription", recipe.description);
      this.my("#recipePortrait").attr("src", recipe.portrait);

      //ingredients
      self.my("#ingredients").html("");
      $.each(recipe.ingredients, function(i, ingredient) {
         var el = $('<div class="ingredient"></div>');

         el.append(ingredient.name_singular + " " + ingredient.count);

         self.my("#ingredients").append(el);

      });
   },

   refreshItemPreviewOLD: function (entityId) {
      // XXX, this uses preview_recipe, which is ONLY necessary when the player
      // chooses a new ingredient material, which may change the recipe's affixes.

      // the recipe's item to be crafted
      console.log("grabbing recipe preview");
      radiant.api.cmd("preview_recipe", {
         crafter_id: this.crafterEntityId,
         recipe_id: entityId,
         using: [],
         components: ["identity", "item_affixes"]
      }).done(function (e) {
         console.log(e);
         var item_preview = e[0];

         // name and desc.
         self.render("#recipeName", item_preview.identity.name);
         self.render("#recipeDescription", item_preview.identity.description);
         self.my("#recipePortrait").attr("src", item_preview.identity.portrait);

         //affixes
         //XXX change me to use EJS
         var affixes = item_preview.item_affixes.affixes;

         if (affixes != undefined) {
            var t = "";
            var numAffixes = 0;
            affixes.forEach(function (affix) {
               var modififer;

               if (affix.modififer == "percent") {
                  modififer = "%";
               } else {
                  modififer = affix.modififer;
               }

               var styleClass = affix.magnitude > 0 ? "buff-text" : "debuff-text";

               t += "<tr>";
               t += "<td class='buff-bubble'><span class=" + styleClass + "><nobr>" + affix.magnitude + " " + modififer + "</nobr></span></td>";
               t += "<td>" + affix.description + "</td>";

            });
         }

         self.my("#recipe-buffs").html(t);

      });
   },

   setupLinkHandlers: function () {
      var v = this.my_element;
      self = this;

      this.my(".recipe-link").click(function (e) {
         self.selectRecipe($(this).attr('recipeKey'));
      });

   },

   updateWorkOptions: function () {

      if (this.my("#work-option-amount").attr("checked")) {
         console.log("showing how many");
         this.my("#how-many-input").show();
         this.my("#craft-when-input").hide();
      } else {
         console.log("showing until");
         this.my("#how-many-input").hide();
         this.my("#craft-when-input").show();
      }
   }
});




