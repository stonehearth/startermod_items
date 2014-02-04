$.widget( "stonehearth.stonehearthMap", {
   // default options
   mapGrid: {},
   options: {
      // callbacks
      hover: null,
      click: null,
      cellSize: 10,
      click: function(cellX, cellY) {
         console.log('you clicked on ' + cellX + ', ' + cellY)
      }
   },

   cellColors: {
      plains_1:    '#927e59',
      plains_2:    '#948a48',
      foothills_1: '#888a4a',
      foothills_2: '#888a4a',
      mountains_1: '#807664',
      mountains_2: '#888071',
      mountains_3: '#948d7f',
      mountains_4: '#aaa59b',
      mountains_5: '#c5c0b5',
      mountains_6: '#d9d5cb',
      mountains_7: '#f2eee3'
   },

   cellHeights: {
      plains_1:    1,
      plains_2:    2,
      foothills_1: 3,
      foothills_2: 4,
      mountains_1: 5,
      mountains_2: 6,
      mountains_3: 7,
      mountains_4: 8,
      mountains_5: 9,
      mountains_6: 10,
      mountains_7: 11
   },

   _create: function() {
      this._buildMap()
   },

   _buildMap: function() {
      var self = this;

      this.element.addClass('stonehearthMap');
      var grid = this.options.mapGrid;
      var canvas = $('<canvas>');
      var overlay = $('<canvas>');
      var container = $('<div>').css('position', 'relative');

      this.mapWidth = this.options.mapGrid[0].length * this.options.cellSize;
      this.mapHeight = this.options.mapGrid.length * this.options.cellSize;

      canvas
         .addClass('mapCanvas')
         .attr('width', this.mapWidth)
         .attr('height', this.mapHeight);

      overlay
         .addClass('mapCanvas')      
         .attr('id', 'overlay')
         .attr('width', this.mapWidth)
         .attr('height', this.mapHeight)
         .click(function(e) {
            self._onMouseClick(self, e);
         })
         .mousemove(self, function(e) {
            self._onMouseMove(self, e);
         });

      this.ctx = canvas[0].getContext('2d');
      this.overlayCtx = overlay[0].getContext('2d');

      this._drawMap();

      container.append(canvas);
      container.append(overlay);

      this.element.append(container);
   },

   _onMouseClick: function (view, e) {
      var self = view;

      var cellSize = self.options.cellSize;
      var cellX = Math.floor(e.offsetX / cellSize);
      var cellY = Math.floor(e.offsetY / cellSize);

      this.options.click(cellX, cellY);
   },

   _onMouseMove: function (view, e) {
      var self = view;

      var cellSize = self.options.cellSize;
      var cellX = Math.floor(e.offsetX / cellSize);
      var cellY = Math.floor(e.offsetY / cellSize);

      if (cellX != self.mouseCellX || cellY != self.mouseCellY) {
         self.overlayCtx.globalAlpha = 0.8;
         self.overlayCtx.clearRect(0, 0, self.mapWidth, self.mapHeight);
         self.overlayCtx.fillStyle = '#ffc000'
         self.overlayCtx.fillRect(
            cellX * cellSize, 
            cellY * cellSize, 
            cellSize, 
            cellSize);

         var lineX = cellX * cellSize + (cellSize / 2)
         var lineY = cellY * cellSize + (cellSize / 2)
         
         this.overlayCtx.lineWidth = 1.0;
         this.overlayCtx.setLineDash([2]);

         this.overlayCtx.beginPath();
         this.overlayCtx.moveTo(0, lineY);
         this.overlayCtx.lineTo(self.mapWidth, lineY);
         this.overlayCtx.stroke();          

         this.overlayCtx.beginPath();
         this.overlayCtx.moveTo(lineX, 0);
         this.overlayCtx.lineTo(lineX, self.mapHeight);
         this.overlayCtx.stroke();          
         self.overlayCtx.globalAlpha = 1.0;
      }

      self.mouseCellX = cellX;
      self.mouseCellY = cellY;
   },

   _drawMap: function() {
      var grid = this.options.mapGrid;
      for (var row = 0; row < grid.length; row++) {
         for (var col=0; col < grid[row].length; col++) {
            this._drawCell(col, row, grid[row][col]);
         }
      }
   },

   _drawCell: function(x, y, cell) {
      var cellSize = this.options.cellSize;
      var cellX = x * cellSize;
      var cellY = y * cellSize


      this.ctx.fillStyle = this.cellColors[cell.terrain_code] ? this.cellColors[cell.terrain_code] : '#000000';
      this.ctx.fillRect(
         cellX, 
         cellY, 
         cellSize,
         cellSize);

      var cellHeight = this._heightAt(x, y)

      if(this._heightAt(x, y - 1) > cellHeight) {
         // north, line above me
         this._drawLine(
            cellX, cellY,
            cellX + cellSize , cellY
         );

         //xxx, shading above me
         this.ctx.globalAlpha = 0.2;
         this.ctx.fillStyle = '#000000';
         this.ctx.fillRect(
            cellX,
            cellY,
            cellSize,
            cellSize * -0.4);
         this.ctx.globalAlpha = 1.0;
      }

      if(this._heightAt(x, y + 1) > cellHeight) {
         // south, line below me
         this._drawLine(
            cellX, cellY + cellSize,
            cellX + cellSize , cellY + cellSize
         );
      }

      if(this._heightAt(x - 1, y) > cellHeight) {
         // east, line on my left
         this._drawLine(
            cellX, cellY,
            cellX, cellY + cellSize
         );
      }

      if(this._heightAt(x + 1, y) > cellHeight) {
         // west, line on my right
         this._drawLine(
            cellX + cellSize, cellY,
            cellX + cellSize , cellY + cellSize
         );
      }

      // forest color #e6e6e6

   },

   _drawLine: function(x1, y1, x2, y2) {
      this.ctx.beginPath();
      this.ctx.moveTo(x1, y1);
      this.ctx.lineTo(x2, y2);
      this.ctx.lineWidth = 0.4;
      this.ctx.stroke();     
   },

   _heightAt: function(x, y) {
      
      if (x < 0 || y < 0 || x >= this.options.mapGrid[0].length || y >= this.options.mapGrid.length) {
         return -1;
      }

      var terrain = this._terrainAt(x, y);

      return this.cellHeights[terrain] 
   },

   _terrainAt: function(x, y) {
      if (x < 0 || y < 0 || x >= this.options.mapGrid[0].length || y >= this.options.mapGrid.length) {
         return '';
      }
      return this.options.mapGrid[y][x].terrain_code;
   },   
});
