$(document).ready(function(){
   $(top).on("radiant_promote_to_profession", function (_, e) {
      var r  = new RadiantTrace()
      var components = { 
            'unit_info' : {},
            'stonehearth:promotion_talisman' : {} 
         };

      // grab the properties from the talisman and pass them along to the promotion wizard
      r.traceUri(e.entity, components)
         .progress(function(eobj) {
               // eobj could be either a talisman or the person to promote              
               var talisman = eobj['stonehearth:promotion_talisman'] ? eobj : null;
               var citizen = !eobj['stonehearth:promotion_talisman'] ? eobj : null;

               App.gameView.addView(App.StonehearthPromotionTree, { 
                  talisman: talisman,
                  citizen: citizen
               });
               r.destroy();
            });
   });
});

App.StonehearthPromotionTree = App.View.extend({
	templateName: 'promotionTree',
   classNames: ['flex', 'fullScreen'],
   components: {
      "jobs" : {
      }
   },

   didInsertElement: function() {
      this._super();

      var self = this;

      self.jobsTrace = new StonehearthDataTrace('stonehearth:professions:index', self.components);
      self.jobsTrace.progress(function(eobj) {
            self._jobs = eobj.jobs;
            self._buildTree();
         });

      //this.set('uri', 'stonehearth:professions:index');

   },

   _buildTree: function() {
      var self = this;

      var content = this.$('#content');
      var svg = this.$('svg');
      var buttonSize = { width: 74, height: 79 };

      // xxx, eventually generate this from some graph layout library like graphvis, if we can
      // get satisfactory results
      var layout = {
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

      var edges = self._buildEdges();

      // draw the edges
      $.each(edges, function(i, edge) {
         var line = document.createElementNS('http://www.w3.org/2000/svg','line');

         line.setAttributeNS(null,'x1', layout[edge.from].x + buttonSize.width / 2);
         line.setAttributeNS(null,'y1', layout[edge.from].y + buttonSize.height / 2);
         line.setAttributeNS(null,'x2', layout[edge.to].x + buttonSize.width / 2);
         line.setAttributeNS(null,'y2', layout[edge.to].y + buttonSize.height / 2);
         line.setAttributeNS(null,'style','stroke:rgb(255,255,255);stroke-width:2');
         svg.append(line);         
      })

      // draw the nodes
      $.each(self._jobs, function(i, job) {
         var l = layout[job.alias];

         var img = document.createElementNS('http://www.w3.org/2000/svg','image');

         img.setAttributeNS('http://www.w3.org/1999/xlink','href', job.buttonIcon);
         img.setAttributeNS(null,'x', l.x);
         img.setAttributeNS(null,'y', l.y);
         img.setAttributeNS(null,'width', buttonSize.width);
         img.setAttributeNS(null,'height', buttonSize.height);
         img.setAttributeNS(null,'class','jobButton');
         svg.append(img);         
        
      });

      self._addTreeHandlers();
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

      this.$('.jobButton').click(function() {
         if(! $(this).hasClass('locked')) {
            var profressionAlias = $(this).attr('id');
            self._promote(profressionAlias);
         }
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

   _promote: function(professionAlias) {
      var self = this;

      var professionInfo = self._getProfessionInfo(professionAlias);

      var citizen = this.get('citizen');
      var talisman = professionInfo.talisman_uri;

      radiant.call('stonehearth:grab_promotion_talisman', citizen.__self, talisman);
      this.destroy();
   },

   destroy: function() {
      if (this.jobsTrace) {
         this.jobsTrace.destroy();
      }
      this._super();
   },


});