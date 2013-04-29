(function ($) {

   $.widget("stonehearth.paginator", {
      options: {
         nav: null,
      },

      _pages: null,
      _currentPage: 0,
      _layout: {
         y: 0,
         pageNum: 0
      },

      add: function (child) {
         ///console.log("adding " + child.attr("id"));
         this.element.append(child);
         var childsPage = this._layoutChild(child);

         if (childsPage != this._currentPage) {
            child.hide();
         }

         this._buildNavigation();
      },

      empty: function () {
         this._pages = [];
         this._pages.push([]); // initial page
         this._layout.y = 0;
         this._layout.pageNum = 0;

         this.element.empty();
      },

      showPage: function (pageNum) {
         if (this._currentPage == pageNum) {
            return;
         }

         this._currentPage = pageNum;

         var y = 0;
         var i = 0;

         for (var i = 0; i < this.element.children().length; i++) {
            var child = $(this.element.children()[i]);
            child.hide();
         }

         for (var i = 0; i < this._pages[pageNum].length; i++) {
            var child = $(this._pages[pageNum][i]);
            child.show();
         }

         this._buildNavigation();
      },

      showLastPage: function () {
         console.log("showing page " + (this._pages.length - 1));
         this.showPage(this._pages.length - 1);
      },

      refresh: function () {
         //console.log("refresh the paginator!");
         //this._calculateLayout();
         //this.showPage(this._currentPage);
      },

      _create: function () {
         this._calculateLayout();
         this.showPage(this._currentPage);
      },

      _calculateLayout: function () {
         this.empty();

         for (var i = 0; i < this.element.children().length; i++) {
            var child = $(this.element.children()[i]);
            this._layoutChild(child);
         }
      },

      _layoutChild: function (child) {
         this._layout.y += child.height();

         if (this._layout.y > this.element.height()) {
            this._pages.push([]); // add a new page
            this._layout.pageNum++;
            this._layout.y = child.height();
         }

         //console.log("putting " + child.attr("id") + " on page " + this._layout.pageNum + "... y = " + this._layout.y + "/" + this.element.height());
         
         this._pages[this._layout.pageNum].push(child);

         return this._layout.pageNum;
      },

      _buildNavigation: function () {
         if (this._pages.length <= 1) {
            return;
         }

         var self = this;
         var nav = $(this.options.nav);

         nav.empty();

         // page links
         for (var i = 0; i < this._pages.length; i++) {
            var link;

            link = $("<div class=paginator-page>" + (i + 1) + "</div>");

            if (i != this._currentPage) {
               link.addClass("paginator-page-link");
               link = $("<a href=" + i + "></a>").append(link);
               link.click(function (e) {
                  e.preventDefault();
                  var n = $(this).attr("href")
                  self.showPage(parseInt(n));
               });


            }

            link
               .css({
                  float: "left"
               })
               .appendTo(nav);

            nav.append(link);
         }

         nav.append("<div style='clear: both;'></div>");

      },

      _dump: function () {

      },

      destroy: function () {
         this.pages = [];
      },

      _setOption: function (option, value) {
         $.Widget.prototype._setOption.apply(this, arguments);
         switch (option) {

         }
      }

   });

})(jQuery);