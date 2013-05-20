radiant.views['stonehearth.views.sidebar.store'] = radiant.View.extend({
   options: {
      position: {
         my: "left top",
         at: "left top",
         of: "#sidebarTab-store"
      },
      deferShow: true
   },

   stockpileList: null,

   onAdd: function (view) {
      this._super(view);

      // build the ui
      $("#createStockpileButton")
         .button()
         .radiantTooltip({
            text: "Workers will move resources and objects into stockpiles for convenient storage",
            hotkey: ""
         });

      this.my("#list").paginator({
         nav: "#nav"
      });

      this.stockpileList = this.my("#list").data("paginator");
      this.my("#search").hide();
      //this.my("#help").distress();  <-- fix this with a style

      this._initHandlers();

      // initial ui state
      this._getStockpiles();
   },

   _initHandlers: function () {
      var self = this;

      // event handlers
      this.my("#createStockpileButton").click(function () {
         console.log("creating stockpile");
         self.createStockpile();
      });

      var s = this.my("#search");
      this.my("#search").keyup(function (e) {
         var input = $(this);
         self.my(".stockpileGrid").itemgrid({
            filter: input.val()
         });
      });

   },

   createStockpile: function () {
      // switch to the store tab if necessary
      var sidebar = radiant.views.controllers["stonehearth.views.sidebar"];
      sidebar.selectTab("store", true);

      radiant.api.execute('mod://stonehearth/actions/create_stockpile.txt')
         .done(function (response) {
            var id = response.entity_id;
            //self.stockpileList.showLastPage(); //XXX doesn't work, because getStockpiles completes later

            // popup the properties window
            /*
            radiant.views.add("stonehearth.views.sidebar.store.stockpileWindow");
            radiant.views.add("views/stockpileWindow", {
               complete: function (view, controller) {
                  controller.setEntityId(id);
               }
            });
            */
         });
   },

   _getStockpiles: function () {
      var self = this;

      radiant.trace_entities()
        .filter("stockpile")
        .get("identity.name")
        .get("stockpile.capacity")
        .getEntityList("stockpile.contents", ["identity", "item.stacks"])
        .progress(function (results) {
           console.log(results);
           self.data = results;
           self._updateView();
        });
   },

   _updateView: function () {
      var self = this;
      var template = $('#stockpileTemplate').html();

      console.log("new stockpile updates");

      this.my("#search").show(); // <-- hacky as fuck
      this.my("#help").hide();

      $.each(this.data, function (entityId, entity) {
         var stockpileGridId = "stockpile_" + entityId;
         var grid = self.my('#'+stockpileGridId);
         if (grid.length > 0) {
            // update existing stockpile
            grid.data("itemgrid").update(entity.stockpile.contents);
         } else {
            // add new stockpile

            var body = $("<div id=" + entityId + "></div>");

            var entry = "<div class=listEntry entity_id=" + entityId + ">";
            entry +=  "<div class='name outlined'><a href=#>" + entity.identity.name + "</a></div>";
            entry += "</div>";

            body.append(entry);

            $("<div class=stockpileGrid id=" + stockpileGridId +"></div>")
               .itemgrid({
                  container: body,
                  cellSize: 32,
                  padding: 2,
                  width: 12,
                  items: entity.stockpile.contents,
                  size: entity.stockpile.capacity,
                  itemClick: function (entityId) {
                     radiant.api.selectEntity(parseInt(entityId));
                  }
               })
               .appendTo(body);

            self.stockpileList.add(body);
         }
      });
      

      this.my(".listEntry").click(function (e) {
         e.preventDefault();
         var entityId = $(this).attr('entity_id');
         radiant.api.selectEntity(parseInt(entityId));
      });

      this.ready();
   },

   clear: function () {
      this.stockpileList.empty();
   }

});
