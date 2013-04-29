
function StonehearthCharacterSheet() {
   this.myElement = null;
   this.url = '/views/character-sheet';
   this.BACKPACK_SIZE = 5;
   this._activePage = null;

   this.init = function (element) {
      this.myElement = element;
      var me = this;
      var v = this.myElement;

      // setup ui elements
      v.dialog("option", "buttons", { Close: function () { $(this).dialog("close"); } });

      v.find(".button").button();
      v.find("#character-sheet-stats-frame").scale9Grid({ top: 40, bottom: 40, left: 40, right: 40 });

      // init event handlers
      this._initHandlers();

      // refresh
      $.getJSON("/api/character-sheet-footman.json", function (data) {
         me.refresh(data.data);
      });

   };

   this.refresh = function (data) {
      radInfo("refreshing character sheet for " + data.name);

      this.clear();

      var v = this.myElement;
      v.hide();

      v.find("#character-sheet-name").html(data.name);
      v.find("#character-sheet-job").html("Level " + data.job.level + " " + data.job.name);


      // main page
      this._addStats(v.find("div#character-sheet-stats table"), data.stats);
      this._addStats(v.find("div#character-sheet-combat-stats table"), data.combatstats);

      this._addEquipment(data.equipment);
      this._addBackpackItems(data.backpack);

      if (this._activePage == null) {
         this._activePage = v.find("#character-sheet-page-main");
         this._refreshActivePage();
      } else {
         setActivePage("main");
      }
      

      this._updateDragDrop();
      v.show();

   };

   this.setActivePage = function (pageSuffix) {
      var v = this.myElement;
      this._activePage = v.find("#character-sheet-page-" + pageSuffix);
      this._refreshActivePage();
   }

   this._refreshActivePage = function () {
      var v = this.myElement;

      v.find("[id^=character-sheet-page]").hide();
      this._activePage.show();
   }

   this._initHandlers = function () {
      var v = this.myElement;
      var me = this;

      v.find("#character-sheet-equipment-button").click(function () {
         me.setActivePage("main");
      });

      v.find("#character-sheet-vitals-button").click(function () {
         me.setActivePage("vitals");
      });

      v.find("#character-sheet-bio-button").click(function () {
         me.setActivePage("bio");
      });

      v.find("#character-sheet-journal-button").click(function () {
         me.setActivePage("journal");
      });

   }

   this._updateDragDrop = function () {
      var v = this.myElement;
      var me = this;

      // draggables and droppables
      v.find(".draggable-item").draggable({
         revert: "invalid"
      });

      
      v.find(".character-sheet-equipment-slot").droppable({
         accept: ".draggable-item",
         hoverClass: "ui-state-active",
         drop: function (event, ui) {
            ui.draggable
               .css("top", "-1px")
               .css("left", "-1px")
               .appendTo($(this));

            me._updateDragDrop();
            //$(this).append(ui.draggable);
            /*
            $(this)
                .addClass("ui-state-highlight");
               */
         }
      });

   }

   this.clear = function () {
      radInfo("************* FUCK!  IMPLEMENT StonehearthCharacterSheet.clear()!!!!!");
   }

   this._addStats = function(table, stats) {
      for (var i = 0; i < stats.length; i++) {
         
         var stat = stats[i];

         var row = '<tr><td class="character-sheet-stat-name">';
         row += '<img src="/views/character-sheet/images/icon-' + stat.name.toLowerCase() + '.png" />' + stat.name + '</td>';
         row += '<td class="character-sheet-stat-value">' + stat.value + '</td></tr>';

         table.append(row);
      }
   }

   this._addEquipment = function (equipment) {
      var v = this.myElement;
      var table = v.find("div#character-sheet-equipment table");

      var sortOrder = ["head", "body", "main-hand", "off-hand", "trinket"];
      var rows = ["", "", "", "", ""];

      for (var i = 0; i < equipment.length; i++) {
         var item = equipment[i];

         var row = this._getItemRow(item);

         for (var slotIndex = 0; slotIndex < sortOrder.length; slotIndex++) {
            if (sortOrder[slotIndex] == item.slot) {
               break;
            }
         }

         rows[slotIndex] = row;
      }

      for (var i = 0; i < rows.length; i++) {
         table.append(rows[i]);
      }
   };

   this._addBackpackItems = function(items) {
      var v = this.myElement;
      var table = v.find("div#character-sheet-backpack table");

      var i;
      for (i = 0; i < this.BACKPACK_SIZE; i++) {
         var item = i < items.length ? items[i] : null;
         table.append(this._getItemRow(item));
      }

      
   }

   this._getItemRow = function (item) {
      var img = "";
      var name = "";

      if (item != null) {
         img = '<img src="' + item.icon + '" class="draggable-item" />';
         name = item.name;
      } 

      var row = '<tr class="character-sheet-equipment-row">';
      row += '<td class="character-sheet-equipment-slot">' + img + '</td>';
      row += '<td class="character-sheet-equipment-name">' + name + '</td></tr>'

      return row;
   }
}




