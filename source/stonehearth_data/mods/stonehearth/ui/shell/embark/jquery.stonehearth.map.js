$.widget( "stonehearth.stonehearthMap", {
   // default options
   mapGrid: {},

   options: {
   },

   _create: function() {
      this._buildMap()
   },

   _buildMap: function() {
      var self = this;

      this.element.addClass('stonehearthMap');

      var mapHtml = '<table>';
      for (var row = 0; row < this.options.mapGrid.length; row++) {
         mapHtml += '<tr>';
         for (var col=0; col < this.options.mapGrid[row].length; col++) {
            mapHtml += this._getTileContent(col, row);
         }
      }

      mapHtml += '</table>';

      this.element.html(mapHtml);

      if (this.options.click) {
         this.element.on( 'click', 'td', function() {
            self.options.click(self._getTileData($(this)));
         });
      }

      if (this.options.mouseover) {
         this.element.on( 'mouseover', 'td', function() {
            self.options.mouseover(self._getTileData($(this)));
         });
      }

      //console.log(JSON.stringify(this.options.mapGrid));
   },

   _getTileData: function(el) {
      var data = {}
      data.terrain = $(el).attr('terrain');
      return data;
   },

   _getTileContent: function(x, y) {
      var content = '';

      var myHeight =this._heightAt(x,y);
      var north = myHeight <= this._heightAt(x, y-1);
      var south = myHeight <= this._heightAt(x, y+1);
      var east  = myHeight <= this._heightAt(x+1, y);
      var west  = myHeight <= this._heightAt(x-1, y);

      var cssClass = 't'
      cssClass += north ? '1' : 0;
      cssClass += south ? '1' : 0;
      cssClass += east ? '1' : 0;
      cssClass += west ? '1' : 0;

      var terrainClass = this._getTerrainClass(x, y);
      var forestDensity = this.options.mapGrid[y][x].forest_density;

      //content += '<div class="' + terrainClass + '">' +'</div>';
      // xxx, below, terrainClass should be the terrain class of the ground "below" the image
      
      content += '<td class=cell terrain=' + terrainClass + '>'; 
      content += '<div class="tile ' + terrainClass + '"><img class="' + cssClass + '"/></div>';
      content += '<div><img class="forest' + forestDensity + '"/></div>';      

      //content += '<div class="colorCell ' + terrainClass + '"></div>';
      content += '<div class="overlay"></div>';

      return content;
   },

   _getTerrainClass: function(x,y) {
      return this._terrainAt(x, y);
   },

   _terrainAt: function(x, y) {
      if (x < 0 || y < 0 || x >= this.options.mapGrid[0].length || y >= this.options.mapGrid.length) {
         return '';
      }
      return this.options.mapGrid[y][x].terrain_code;
   },

   _heightAt: function(x, y) {
      
      if (x < 0 || y < 0 || x >= this.options.mapGrid[0].length || y >= this.options.mapGrid.length) {
         return -1;
      }

      var terrain = this._terrainAt(x, y);

      var heights = {
         'plains_1'    : 1,
         'plains_2'    : 2,
         'foothills_1' : 3,
         'foothills_2' : 4,
         'mountains_1' : 5,
         'mountains_2' : 6,
         'mountains_3' : 7,
         'mountains_4' : 8,
         'mountains_5' : 9,
         'mountains_6' : 10,
      }

      return heights[terrain];
   }
});
