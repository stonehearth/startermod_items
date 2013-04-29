(function ($) {

   $.widget("stonehearth.itemgrid", {
      options: {
         width: 8,
         cellSize: 48,
         padding: 3,
         items: null,
         itemClick: null,
         filter: null,
         container: null,
      },

      _grid: null,
      _items: null,
      _numItems: 0,

      update: function (items) {
         this._removeItems();
         this._updateItems(items);
      },

      _create: function () {
         this._addGrid();
         this._updateItems(this.options.items);
      },

      _addGrid: function () {
         var self = this;
         var o = self.options;
         var el = self.element;
         var size = o.cellSize + o.padding;

         this._grid = $("<div class=itemGrid></div>");

         this._grid.css({
            width: o.width * size
            //height: (parseInt(o.size / o.width) + 1) * size,
         });

         for (var i = 0; i < o.size; i++) {
            var cell = $("<div class=itemGridCell></div>");
            cell.css({
               //position: "absolute",
               //top: parseInt(i / o.width) * size,
               //left: (i % o.width) * size,
               float: "left",
               backgroundSize: o.cellSize + "px, " + o.cellSize + "px",
               backgroundRepeat: "no-repeat",
               width: o.cellSize,
               height: o.cellSize,
               paddingRight: o.padding,
               paddingBottom: o.padding
            })
            .appendTo(this._grid);
         }

         el.append(this._grid);
         el.append('<div style="clear:both"></div>');
      },

      _getItems: function () {
         var self = this;
      },

      _clearItems: function () {
         this._grid.find(".itemGridItem").unbind("click");
         this._grid.find(".itemGridCell").html("");
         this._numItems = 0;
      },

      _removeItems: function () {
         this._grid.find(".itemGridItem").unbind("click");
         this._grid.find(".itemGridItem").remove();
         this._numItems = 0;
      },

      _updateItems: function (items) {
         var o = this.options;
         var itemOpacity;
         var inFilter = true;

         // fill in the items
         for (var key in items) {
            var id = key;
            var data = items[key];

            // check the filter
            inFilter = !(o.filter != null && data.identity.name.toLowerCase().indexOf(o.filter) == -1);

            if (inFilter) {
               itemOpacity = 1.0;
            } else {
               itemOpacity = 0.3;
            }

            // grab the empty cell
            var cell = $(this._grid.children()[this._numItems]);
            cell.css({
               width: o.cellSize,
               height: o.cellSize,
            });

            // add the item div
            var item = $("<div class=itemGridItem entity_id=" + id + "></div>");
            item
               .css({
                  width: o.cellSize,
                  height: o.cellSize,
                  opacity: itemOpacity,
               })
               .click(function (e) {
                  o.itemClick(id);
               });

            // add the icon
            $("<img src=" + data.identity.portrait + " />")
               .css({
                  width: o.cellSize,
                  height: o.cellSize
               })
               .appendTo(item);

            // add the stack count
            if (data.item.stacks > 1) {
               $("<span class='stackCount outlined'>" + data.item.stacks + "</span>")
                  .appendTo(item);
            }

            item.radiantTooltip({
               title: data.identity.name
            })
            .appendTo(cell);

            this._numItems++;

            /*
            var d = numVisibleItems == 0 ? "none" : "block";
            
            this._grid.css({
               display: d
            });
   
            if (o.container != null) {
               o.container.css({
                  display: d
               });
            }
            */
         }
      },

      destroy: function () {
         this._clearItems();
         this.element.html("");
      },

      _setOption: function (option, value) {
         $.Widget.prototype._setOption.apply(this, arguments);
         switch (option) {
            case "filter":
               this._addItems();
               break;
            case "designation":
               this._destroy();
               this._create();
               break;
         }
      }

   });

})(jQuery);