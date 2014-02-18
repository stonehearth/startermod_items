$.widget( "stonehearth.stonehearthMap", {
   // default options
   mapGrid: {},
   options: {
      // callbacks
      hover: null,
      cellSize: 12,
      click: function(cellX, cellY) {
         console.log('Selected cell: ' + cellX + ', ' + cellY)
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

   forestMargin: {
      0: null,
      1: 4,
      2: 3,
      3: 2,
      4: 1,
   },

   setMap: function(mapGrid) {
      var self = this;

      self.options.mapGrid = mapGrid;
      self._drawMap(self.mapContext);
   },

   getMap: function() {
      var self = this;
      return self.options.mapGrid;
   },

   suspend: function() {
      var self = this;

      self._suspended = true;
      self._enableCursor();
   },

   resume: function() {
      var self = this;

      self._suspended = false;
      self._disableCursor();
   },

   suspended: function() {
      var self = this;

      return self._suspended;   
   },

   clearCrosshairs: function() {
      var self = this;

      self._clearCrosshairs(self.overlayContext);
   },

   _create: function() {
      var self = this;

      self._suspended = false;
      self._buildMap();
   },

   _buildMap: function() {
      var self = this;

      self.element.addClass('stonehearthMap');
      var grid = self.options.mapGrid;
      var canvas = $('<canvas>');
      var overlay = $('<canvas>');
      var container = $('<div>').css('position', 'relative');

      self.mapWidth = self.options.mapGrid[0].length * self.options.cellSize;
      self.mapHeight = self.options.mapGrid.length * self.options.cellSize;

      canvas
         .addClass('mapCanvas')
         .attr('width', self.mapWidth)
         .attr('height', self.mapHeight);

      overlay
         .addClass('mapCanvas')
         .addClass('noCursor')
         .attr('id', 'overlay')
         .attr('width', self.mapWidth)
         .attr('height', self.mapHeight)

         .click(function(e) {
            self._onMouseClick(self, e);
         })

         .mousemove(self, function(e) {
            self._onMouseMove(self, e);
         });

      self.mapContext = canvas[0].getContext('2d');
      self.overlayContext = overlay[0].getContext('2d');

      self._drawMap(self.mapContext);

      container.append(canvas);
      container.append(overlay);

      self.element.append(container);
   },

   _onMouseClick: function(view, e) {
      var self = view;

      if (self._suspended) {
         return;
      }

      var cellSize = self.options.cellSize;
      var cellX = Math.floor(e.offsetX / cellSize);
      var cellY = Math.floor(e.offsetY / cellSize);

      self.options.click(cellX, cellY);
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:sub_menu' );
   },

   _onMouseMove: function(view, e) {
      var self = view;

      if (self._suspended) {
         return;
      }

      var cellSize = self.options.cellSize;
      var cellX = Math.floor(e.offsetX / cellSize);
      var cellY = Math.floor(e.offsetY / cellSize);

      if (cellX != self.mouseCellX || cellY != self.mouseCellY) {
         self.mouseCellX = cellX;
         self.mouseCellY = cellY;

         self._drawCrosshairs(self.overlayContext, cellX, cellY);

         self.options.hover(cellX, cellY);
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:action_hover' );
      }
   },

   _drawCrosshairs: function(context, cellX, cellY) {
      var self = this;
      var cellSize = self.options.cellSize;

      self._clearCrosshairs(context);

      context.globalAlpha = 0.8;
      context.clearRect(0, 0, self.mapWidth, self.mapHeight);
      context.fillStyle = '#ffc000'
      context.fillRect(
         cellX * cellSize, cellY * cellSize, 
         cellSize, cellSize
      );

      var lineX = cellX*cellSize + cellSize/2
      var lineY = cellY*cellSize + cellSize/2
      
      context.lineWidth = 1.0;
      context.setLineDash([2]);

      self._drawLine(
         context,
         0, lineY,
         lineX - cellSize/2, lineY
      );

      self._drawLine(
         context,
         lineX + cellSize/2, lineY,
         self.mapWidth, lineY
      );

      self._drawLine(
         context,
         lineX, 0,
         lineX, lineY - cellSize/2
      );

      self._drawLine(
         context,
         lineX, lineY + cellSize/2,
         lineX, self.mapHeight
      );

      context.globalAlpha = 1.0;
   },

   _clearCrosshairs: function(context) {
      var self = this;
      context.clearRect(0, 0, self.mapWidth, self.mapHeight);
   },

   _drawMap: function(context) {
      var self = this;

      var grid = self.options.mapGrid;
      for (var y = 0; y < grid.length; y++) {
         for (var x = 0; x < grid[y].length; x++) {
            self._drawCell(context, x, y, grid[y][x]);
         }
      }
   },

   _drawCell: function(context, cellX, cellY, cell) {
      var self = this;
      var cellSize = self.options.cellSize;
      var x = cellX * cellSize;
      var y = cellY * cellSize;

      // draw elevation
      context.fillStyle = self.cellColors[cell.terrain_code] ? self.cellColors[cell.terrain_code] : '#000000';
      context.fillRect(
         x, y, 
         cellSize, cellSize
      );

      var cellHeight = self._heightAt(cellX, cellY)
      context.lineWidth = 0.4;

      // draw edges for elevation changes
      if(self._heightAt(cellX, cellY - 1) > cellHeight) {
         // north, line above me
         self._drawLine(
            context,
            x, y,
            x + cellSize, y
         );

         //xxx, shading above me
         context.globalAlpha = 0.2;
         context.fillStyle = '#000000';
         context.fillRect(
            x, y,
            cellSize, cellSize * -0.4
         );
         context.globalAlpha = 1.0;
      }

      if(self._heightAt(cellX, cellY + 1) > cellHeight) {
         // south, line below me
         self._drawLine(
            context,
            x, y + cellSize,
            x + cellSize, y + cellSize
         );
      }

      if(self._heightAt(cellX - 1, cellY) > cellHeight) {
         // east, line on my left
         self._drawLine(
            context,
            x, y,
            x, y + cellSize
         );
      }

      if(self._heightAt(cellX + 1, cellY) > cellHeight) {
         // west, line on my right
         self._drawLine(
            context,
            x + cellSize, y,
            x + cellSize, y + cellSize
         );
      }

      // overlay forest
      var forest_density = self._forestAt(cellX, cellY)

      if (forest_density > 0) {
         var margin = self.forestMargin[forest_density];
         context.fillStyle = '#2a582a';
         context.globalAlpha = 0.6;

         context.fillRect(
            x + margin, y + margin, 
            cellSize - margin*2, cellSize - margin*2
         );
         context.globalAlpha = 1.0;
      }
   },

   _drawLine: function(context, x1, y1, x2, y2) {
      context.beginPath();
      context.moveTo(x1, y1);
      context.lineTo(x2, y2);
      context.stroke();     
   },

   _enableCursor: function() {
      $('#overlay').removeClass('noCursor');
      $('#overlay').addClass('arrowCursor');
   },

   _disableCursor: function() {
      $('#overlay').removeClass('arrowCursor');
      $('#overlay').addClass('noCursor');
   },

   _heightAt: function(cellX, cellY) {
      var self = this;

      if (!self._inBounds(cellX, cellY)) {
         return -1;
      }

      var terrain = self._terrainAt(cellX, cellY);

      return self.cellHeights[terrain] 
   },

   _terrainAt: function(cellX, cellY) {
      var self = this;

      if (!self._inBounds(cellX, cellY)) {
         return '';
      }
      return self.options.mapGrid[cellY][cellX].terrain_code;
   },

   _forestAt: function(cellX, cellY) {
      var self = this;

      if (!self._inBounds(cellX, cellY)) {
         return -1;
      }

      return self.options.mapGrid[cellY][cellX].forest_density;
   },

   _inBounds: function(cellX, cellY) {
      var self = this;

      if (cellX < 0 || cellY < 0) {
         return false;
      }

      if (cellX >= self.options.mapGrid[0].length || cellY >= self.options.mapGrid.length) {
         return false;
      }

      return true;
   },
});
