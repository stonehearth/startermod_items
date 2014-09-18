$(document).ready(function(){
   $(top).on("radiant_promote_to_profession", function (_, e) {

      App.gameView.addView(App.StonehearthPromotionTree, { 
         citizen: e.entity
      });
   });
});

App.StonehearthPromotionTree = App.View.extend({
	templateName: 'promotionTree',
   classNames: ['flex', 'fullScreen'],
   closeOnEsc: true,

   components: {
      "jobs" : {}
   },

   didInsertElement: function() {
      this._super();

      var self = this;

      self._jobButtons = {};

      self.jobsTrace = new StonehearthDataTrace('stonehearth:professions:index', self.components);
      self.jobsTrace.progress(function(eobj) {
            self._jobs = eobj.jobs;
            self._initCitizen();
            
         });
   },

   _initCitizen: function() {
      var self = this;
      self._citizenTrace = new StonehearthDataTrace(this.get('citizen'), { 'stonehearth:profession' : {} });
      self._citizenTrace.progress(function(o) {
            self._startingJob = o['stonehearth:profession'].profession_uri;
            self._buildTree();
            self._citizenTrace.destroy();
         })
   },

   _buildTree: function() {
      var self = this;

      var content = this.$('#content');
      var buttonSize = { width: 74, height: 79 };

      self._svg = this.$('svg');

      // xxx, eventually generate this from some graph layout library like graphvis, if we can
      // get satisfactory results
      self._layout = {
         'stonehearth:professions:worker' : {x: 372 , y: 244},
         'stonehearth:professions:carpenter' : {x: 542 , y: 159},
         'stonehearth:professions:mason' : {x: 542 , y: 244},
         'stonehearth:professions:blacksmith' : {x: 542 , y: 329},
         'stonehearth:professions:weaponsmith' : {x: 542 , y: 414},
         'stonehearth:professions:architect' : {x: 627 , y: 74},
         'stonehearth:professions:geomancer' : {x: 712 , y: 159},
         'stonehearth:professions:armorsmith' : {x: 627, y: 329},
         'stonehearth:professions:engineer' : {x: 627 , y: 414 },
         'stonehearth:professions:footman' : {x: 330 , y: 117},
         'stonehearth:professions:archer' : {x: 415 , y: 117},
         'stonehearth:professions:shield_bearer' : {x: 330, y: 32},
         'stonehearth:professions:brewer' : {x: 202 , y: 74},
         'stonehearth:professions:farmer' : {x: 202, y: 159},
         'stonehearth:professions:cook' : {x: 117, y: 159},
         'stonehearth:professions:trapper' : {x: 202 , y: 329},
         'stonehearth:professions:shepherd' : {x: 117 , y: 287},
         'stonehearth:professions:animal_trainer' : {x: 32 , y: 287},
         'stonehearth:professions:hunter' : {x: 117, y: 372},
         'stonehearth:professions:big_game_hunter' : {x: 32, y: 372},
         'stonehearth:professions:miner' : {x: 330, y: 372},
         'stonehearth:professions:weaver' : {x: 415, y: 372},
         'stonehearth:professions:treasure_hunter' : {x: 330, y: 457},
      }

      self._edges = self._buildEdges();

      // draw the edges
      $.each(self._edges, function(i, edge) {
         var line = document.createElementNS('http://www.w3.org/2000/svg','line');

         line.setAttributeNS(null,'x1', self._layout[edge.from].x + buttonSize.width / 2);
         line.setAttributeNS(null,'y1', self._layout[edge.from].y + buttonSize.height / 2);
         line.setAttributeNS(null,'x2', self._layout[edge.to].x + buttonSize.width / 2);
         line.setAttributeNS(null,'y2', self._layout[edge.to].y + buttonSize.height / 2);
         line.setAttributeNS(null,'style','stroke:rgb(255,255,255);stroke-width:2');
         self._svg.append(line);         
      })

      // draw the nodes
      $.each(self._jobs, function(i, job) {
         var l = self._layout[job.alias];

         var button = $('<div>')
            .addClass('jobButton')
            .attr('id', job.alias)
            .append('<img src=' + job.icon + '/>');

         self._jobButtons[job.alias] = button;
      self._addDivToGraph(button, l.x, l.y, 74, 74);
      });

      // unlock nodes based on talismans available in the world
      radiant.call('stonehearth:get_talismans_in_explored_region')
         .done(function(o) {
            $.each(o.available_professions, function(key, profession) {
               self._jobButtons[profession].addClass('available');
            })
         });  

      self._jobButtons[self._startingJob].addClass('available');


      var cursor = $("<div class='jobButtonCursor'></div>");
      self._jobCursor = self._addDivToGraph(cursor, 0, 0, 84, 82);
      self._addTreeHandlers();
      self._updateUi(this._startingJob);
   },


   // from: http://stackoverflow.com/questions/12462036/dynamically-insert-foreignobject-into-svg-with-jquery
   _addDivToGraph: function(div, x, y, width, height) {
      var self = this;
      var foreignObject = document.createElementNS('http://www.w3.org/2000/svg', 'foreignObject' );
      var body = document.createElement( 'body' ); 
      $(foreignObject)
         .attr("x", x)
         .attr("y", y)
         .attr("width", width)
         .attr("height", height).append(body);
      
      $(body)
         .append(div);

      self._svg.append(foreignObject);                  

      return foreignObject;
   },

   _buildEdges: function() {
      var edges = [];

      $.each(this._jobs, function(i, job) {
         var parent = job.parent_profession || 'stonehearth:professions:worker';
         edges.push({
            from: job.alias,
            to: parent
         })
      });

      return edges;
   },

   _addTreeHandlers: function() {
      var self = this;

      self.$('.jobButton').click(function() {
         var jobAlias = $(this).attr('id');
         self._updateUi(jobAlias);
      });

      self.$('#promoteButton').click(function() {
         self._promote(self.get('selectedJob.alias'));
      })
   },

   _getProfessionInfo: function(id) {
      var ret;
      $.each(this._jobs, function(i, job) {
         if (job.alias == id) {
            ret = job;
         }
      })

      return ret;
   },

   _updateUi: function(jobAlias) {
      var self = this;
      var selectedJob;

      $.each(self._jobs, function(i, job) {
         if (job.alias == jobAlias) {
            selectedJob = job;
         }
      })

      // move the cursor
      $(self._jobCursor)
         .attr('x', self._layout[jobAlias].x - 5)
         .attr('y', self._layout[jobAlias].y - 4);

      // tell handlebars about changes
      self.set('selectedJob', selectedJob);
      self.set('promoteButtonText', 'Promote to ' + selectedJob.name); //xxx localize

      var requirementsMet = self._jobButtons[jobAlias].hasClass('available') || selectedJob.alias == self._startingJob;
      self.set('requirementsMet', requirementsMet);
      self.set('promoteOk', selectedJob.alias != self._startingJob && requirementsMet);
   },


   _promote: function(jobAlias) {
      var self = this;

      var professionInfo = self._getProfessionInfo(jobAlias);

      var citizen = this.get('citizen');
      var talisman = professionInfo.talisman_uri;

      radiant.call('stonehearth:grab_promotion_talisman', citizen, talisman);
      this.destroy();
   },

   destroy: function() {
      if (this.jobsTrace) {
         this.jobsTrace.destroy();
      }

      if (this._citizenTrace) {
         this._citizenTrace.destroy();
      }

      this._super();
   },


});