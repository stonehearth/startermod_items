/*
* Copyright (c) 2012 by Joseph Marshall
*
* Parts of this code were originally written by Steven Fortune.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose without fee is hereby granted, provided that this entire notice
* is included in all copies of any software which is or includes a copy
* or modification of this software and in all copies of the supporting
* documentation for such software.
* THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
* WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
* REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
* OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
*/

#include "pch.h"
#include "voronoi.h"
#include <algorithm>

#define le 0
#define re 1

// disable 'possible loss of data' when doing automatic conversions between int/float
#pragma warning( push )
#pragma warning( disable : 4244 )

using namespace radiant;
using namespace radiant::csg;

Voronoi::Voronoi(std::vector<Point2> &points,
                 float minX, float maxX, float minY, float maxY) :
    sites(points.size()),
    nsites(sites.size()),
    hf1(), sf1(), ef1(),
    ELhash(),
    PQhash(),
    PQcount(0),
    PQmin(0),
    ELleftend(HEcreate(nullptr, 0)),
    ELrightend(HEcreate(nullptr, 0)),
    xmin(0), xmax(0),
    ymin(0), ymax(0),
    deltay(0), deltax(0),
    bottomsite(nullptr),
    borderMinX(minX), borderMaxX(maxX), borderMinY(minY), borderMaxY(maxY)
{
   int sqrt_nsites = std::ceil(std::sqrt(nsites+4));
    ELhash.resize(2*sqrt_nsites);
    PQhash.resize(4*sqrt_nsites);
    ELleftend->ELright = ELrightend;
    ELrightend->ELleft = ELleftend;

    ELhash[0] = ELleftend;
    ELhash[ELhash.size()-1] = ELrightend;


    xmax = xmin = points[0].x;
    ymax = ymin = points[0].y;

    int i = 0;
    for (auto &s : sites) {
        int x = s.coord.x = points[i].x;
        int y = s.coord.y = points[i].y;
        i++;

        if(x < xmin)
            xmin = x;
        else if(x > xmax)
            xmax = x;

        if(y < ymin)
            ymin = y;
        else if(y > ymax)
            ymax = y;
    }
    
    sites.sort(scomp);
    next_site = sites.begin();
    
    deltay = ymax - ymin;
    deltax = xmax - xmin;

    voronoi();
}


Halfedge* Voronoi::HEcreate(VEdge *e, int pm) {
    hf1.emplace_back(e, pm);
    return &hf1.back();
}


void Voronoi::ELinsert(Halfedge *lb, Halfedge *newHe) {
    newHe -> ELleft = lb;
    newHe -> ELright = lb -> ELright;
    (lb -> ELright) -> ELleft = newHe;
    lb -> ELright = newHe;
}

/* Get entry from hash table, pruning any deleted nodes */
// TODO Use stl hash
Halfedge * Voronoi::ELgethash(int b) {
    Halfedge *he;
    
    if (b<0 || b>=(int)ELhash.size()) 
        return nullptr;

    he = ELhash[b]; 
    if (he == nullptr || !he->deleted)
        return he;
    
    /* Hash table points to deleted half VEdge.  Patch as necessary. */
    ELhash[b] = nullptr;
    return nullptr;
}	

Halfedge * Voronoi::ELleftbnd(Point2f *p) {
    int i, bucket;
    Halfedge *he;
    

    int ELhashsize = ELhash.size();

    bucket = (p->x - xmin)/deltax * ELhashsize;

    if (bucket < 0) bucket =0;
    if (bucket >= ELhashsize) bucket = ELhashsize - 1;

    he = ELgethash(bucket);
    if (he == nullptr) {
        for (i=1; 1; i += 1) {	
            if ((he=ELgethash(bucket-i)) != nullptr)
                break;
            if ((he=ELgethash(bucket+i)) != nullptr)
                break;
        };
    };


    if (he==ELleftend  || (he != ELrightend && right_of(he,p))) {
        do {
            he = he -> ELright;
        } while (he!=ELrightend && right_of(he,p));	

        he = he -> ELleft;
    } else {
        do {
            he = he->ELleft;
        } while (he!=ELleftend && !right_of(he,p));
    }
        
    if (bucket > 0 && bucket < ELhashsize-1) {	
        ELhash[bucket] = he;
    };
    return he;
}


/* This delete routine can't reclaim node, since pointers from hash
table may be present.   */
void Voronoi::ELdelete(Halfedge *he) {
    (he->ELleft)->ELright = he->ELright;
    (he->ELright)->ELleft = he->ELleft;
    he->deleted = true;
}


VSite * Voronoi::leftreg(Halfedge *he) {
    if(he -> ELedge == nullptr) 
        return(bottomsite);
    return( he -> ELpm == le ? 
        he -> ELedge -> reg[le] : he -> ELedge -> reg[re]);
}

VSite * Voronoi::rightreg(Halfedge *he) {
    if (he->ELedge == nullptr) 
        return bottomsite;
    return he->ELpm == le ? he->ELedge->reg[re] : he->ELedge->reg[le];
}


VEdge * Voronoi::bisect(VSite *s1, VSite *s2) {
    float dx,dy,adx,ady;
    VEdge *newedge;	

    ef1.emplace_back();
    newedge = &ef1.back();
    
    newedge -> reg[0] = s1;
    newedge -> reg[1] = s2;
    //ref(s1); 
    //ref(s2);
    newedge -> ep[0] = nullptr;
    newedge -> ep[1] = nullptr;
    
    dx = s2->coord.x - s1->coord.x;
    dy = s2->coord.y - s1->coord.y;
    adx = dx>0 ? dx : -dx;
    ady = dy>0 ? dy : -dy;
    newedge -> c = (float)(s1->coord.x * dx + s1->coord.y * dy + (dx*dx + dy*dy)*0.5);

    if (adx>ady) {	
        newedge -> a = 1.0; newedge -> b = dy/dx; newedge -> c /= dx;
    } else {	
        newedge -> b = 1.0; newedge -> a = dx/dy; newedge -> c /= dy;
    };

    return newedge;
}

VSite * Voronoi::intersect(Halfedge *el1, Halfedge *el2, Point2f *p) {
    VEdge *e1,*e2, *e;
    Halfedge *el;
    float d, xint, yint;
    int right_of_site;
    VSite *v;
    
    e1 = el1 -> ELedge;
    e2 = el2 -> ELedge;
    if(e1 == nullptr || e2 == nullptr) 
        return nullptr;

    if (e1->reg[1] == e2->reg[1]) 
        return nullptr;
    
    d = e1->a * e2->b - e1->b * e2->a;
    if (-1.0e-10<d && d<1.0e-10) 
        return nullptr;
    
    xint = (e1->c*e2->b - e2->c*e1->b)/d;
    yint = (e2->c*e1->a - e1->c*e2->a)/d;
    
    if( (e1->reg[1]->coord.y < e2->reg[1]->coord.y) ||
        (e1->reg[1]->coord.y == e2->reg[1]->coord.y &&
        e1->reg[1]->coord.x < e2->reg[1]->coord.x) )
    {	
        el = el1; 
        e = e1;
    }
    else
    {	
        el = el2; 
        e = e2;
    };
    
    right_of_site = xint >= e -> reg[1] -> coord.x;
    if ((right_of_site && el -> ELpm == le) || (!right_of_site && el -> ELpm == re)) 
        return nullptr;
    
    sf1.emplace_back();
    v = &sf1.back();
    v -> coord.x = xint;
    v -> coord.y = yint;
    return(v);
}

int Voronoi::right_of(Halfedge *el, Point2f *p) {
    VEdge *e;
    VSite *topsite;
    int right_of_site, above, fast;
    float dxp, dyp, dxs, t1, t2, t3, yl;
    
    e = el -> ELedge;
    topsite = e -> reg[1];
    right_of_site = p -> x > topsite -> coord.x;
    if(right_of_site && el -> ELpm == le) return(1);
    if(!right_of_site && el -> ELpm == re) return (0);
    
    if (e->a == 1.0) {
        dyp = p->y - topsite->coord.y;
        dxp = p->x - topsite->coord.x;
        fast = 0;
        if ((!right_of_site & (e->b<0.0)) | (right_of_site & (e->b>=0.0)) ) {
            above = dyp>= e->b*dxp;	
            fast = above;
        } else {
            above = p->x + p->y*e->b > e-> c;
            if (e->b<0.0) above = !above;
            if (!above) fast = 1;
        }
        if (!fast) {
            dxs = topsite->coord.x - (e->reg[0])->coord.x;
            above = e->b * (dxp*dxp - dyp*dyp) <
                dxs*dyp*(1.0+2.0*dxp/dxs + e->b*e->b);
            if (e->b<0.0) above = !above;
        }
    } else {
        yl = e->c - e->a*p->x;
        t1 = p->y - yl;
        t2 = p->x - topsite->coord.x;
        t3 = yl - topsite->coord.y;
        above = t1*t1 > t2*t2 + t3*t3;
    }
    return (el->ELpm==le ? above : !above);
}


void Voronoi::endpoint(VEdge *e, int lr, VSite * s) {
    e->ep[lr] = s;
    //ref(s);
    if(e->ep[re-lr] == nullptr) 
        return;

    clip_line(e);
}


float Voronoi::dist(VSite *s, VSite *t) {
    float dx,dy;
    dx = s->coord.x - t->coord.x;
    dy = s->coord.y - t->coord.y;
    return (float)(sqrt(dx*dx + dy*dy));
}

void Voronoi::PQinsert(Halfedge *he,VSite * v, float offset) {
    Halfedge *last, *next;
    
    he -> vertex = v;
    //ref(v);
    he -> ystar = (float)(v -> coord.y + offset);
    last = &PQhash[PQbucket(he)];
    while ((next = last -> PQnext) != nullptr &&
        (he -> ystar  > next -> ystar  ||
        (he -> ystar == next -> ystar && v -> coord.x > next->vertex->coord.x)))
    {	
        last = next;
    };
    he -> PQnext = last -> PQnext; 
    last -> PQnext = he;
    PQcount += 1;
}

void Voronoi::PQdelete(Halfedge *he) {
    Halfedge *last;
    
    if(he -> vertex != nullptr) {
        last = &PQhash[PQbucket(he)];
        while (last -> PQnext != he) 
            last = last -> PQnext;

        last -> PQnext = he -> PQnext;
        PQcount -= 1;
        he -> vertex = nullptr;
    };
}

int Voronoi::PQbucket(Halfedge *he) {
    int bucket;
    int PQhashsize = PQhash.size();
    
    bucket = (int)((he->ystar - ymin)/deltay * PQhashsize);
    if (bucket<0) bucket = 0;
    if (bucket>=PQhashsize) bucket = PQhashsize-1 ;
    if (bucket < PQmin) PQmin = bucket;
    return(bucket);
}



int Voronoi::PQempty() {
    return(PQcount==0);
}


Point2f Voronoi::PQ_min() {
    PQmin = 0;
    for (auto &e : PQhash) {
        if (e.PQnext != nullptr) {
            Point2f p;
            p.x = e.PQnext -> vertex -> coord.x;
            p.y = e.PQnext -> ystar;
            return p;
        }
        PQmin ++;
    }
    throw "No PQ found in PQ_min!";
}

Halfedge * Voronoi::PQextractmin() {
    Halfedge *curr;
    
    curr = PQhash[PQmin].PQnext;
    PQhash[PQmin].PQnext = curr -> PQnext;
    PQcount -= 1;
    return(curr);
}

void Voronoi::clip_line(VEdge *e) {
    VSite *s1, *s2;
    float x1=0,x2=0,y1=0,y2=0, temp = 0;

    x1 = e->reg[0]->coord.x;
    x2 = e->reg[1]->coord.x;
    y1 = e->reg[0]->coord.y;
    y2 = e->reg[1]->coord.y;

    int pxmin = borderMinX;
    int pxmax = borderMaxX;
    int pymin = borderMinY;
    int pymax = borderMaxY;

    if (e -> a == 1.0 && e ->b >= 0.0) {	
        s1 = e -> ep[1];
        s2 = e -> ep[0];
    } else {
        s1 = e -> ep[0];
        s2 = e -> ep[1];
    };
    
    if (e -> a == 1.0) {
        y1 = pymin;
        if (s1!=nullptr && s1->coord.y > pymin)
            y1 = s1->coord.y;
        if (y1>pymax)
            y1 = pymax;
        x1 = e -> c - e -> b * y1;
        y2 = pymax;
        if (s2!=nullptr && s2->coord.y < pymax) 
            y2 = s2->coord.y;

        if (y2<pymin) 
            y2 = pymin;
        x2 = (e->c) - (e->b) * y2;
        if (((x1> pxmax) & (x2>pxmax)) | ((x1<pxmin)&(x2<pxmin))) 
            return;
        if (x1> pxmax) {
            x1 = pxmax; y1 = (e -> c - x1)/e -> b;
        }
        if (x1<pxmin) {
            x1 = pxmin; y1 = (e -> c - x1)/e -> b;
        }
        if (x2>pxmax) {
            x2 = pxmax; y2 = (e -> c - x2)/e -> b;
        }
        if (x2<pxmin) {
            x2 = pxmin; y2 = (e -> c - x2)/e -> b;
        }
    } else {
        x1 = pxmin;
        if (s1!=nullptr && s1->coord.x > pxmin) 
            x1 = s1->coord.x;
        if (x1>pxmax) 
            x1 = pxmax;
        y1 = e -> c - e -> a * x1;
        x2 = pxmax;
        if (s2!=nullptr && s2->coord.x < pxmax) 
            x2 = s2->coord.x;
        if (x2<pxmin)
            x2 = pxmin;
        y2 = e -> c - e -> a * x2;
        if (((y1> pymax) & (y2>pymax)) | ((y1<pymin)&(y2<pymin))) {
            return;
        }
        if (y1> pymax) {
            y1 = pymax; x1 = (e -> c - y1)/e -> a;
        }
        if (y1<pymin) {
            y1 = pymin; x1 = (e -> c - y1)/e -> a;
        }
        if (y2>pymax) {
            y2 = pymax; x2 = (e -> c - y2)/e -> a;
        }
        if (y2<pymin) {
            y2 = pymin; x2 = (e -> c - y2)/e -> a;
        }
    }
    
    if (!(x1 == x2 && y1==y2)) {
        e->reg[0]->edges.push_back(VSitePoints(x1, y1, x2, y2));
        e->reg[1]->edges.push_back(VSitePoints(x1, y1, x2, y2));
    }
}

bool Voronoi::voronoi() {
    VSite *newsite, *bot, *top, *temp, *p;
    VSite *v;
    Point2f newintstar;
    int pm;
    Halfedge *lbnd, *rbnd, *llbnd, *rrbnd, *bisector;
    VEdge *e;
    
    bottomsite = nextone();
    newsite = nextone();

    while(1) {
        if(!PQempty()) 
            newintstar = PQ_min();
        
        if (newsite != nullptr && (
                PQempty() || newsite -> coord.y < newintstar.y
                || (newsite->coord.y == newintstar.y 
                    && newsite->coord.x < newintstar.x))) {

            lbnd = ELleftbnd(&(newsite->coord));
            rbnd = lbnd->ELright;
            bot = rightreg(lbnd);
            e = bisect(bot, newsite);
            bisector = HEcreate(e, le);
            ELinsert(lbnd, bisector);
            if ((p = intersect(lbnd, bisector)) != nullptr) {	
                PQdelete(lbnd);
                PQinsert(lbnd, p, dist(p,newsite));
            };

            lbnd = bisector;						
            bisector = HEcreate(e, re);
            ELinsert(lbnd, bisector);
            if ((p = intersect(bisector, rbnd)) != nullptr) {	
                PQinsert(bisector, p, dist(p,newsite));
            };
            newsite = nextone();	

        } else if (!PQempty()) {
            lbnd = PQextractmin();
            llbnd = lbnd->ELleft;
            rbnd = lbnd->ELright;
            rrbnd = rbnd->ELright;
            bot = leftreg(lbnd);
            top = rightreg(rbnd);
            v = lbnd->vertex;
            endpoint(lbnd->ELedge,lbnd->ELpm,v);
            endpoint(rbnd->ELedge,rbnd->ELpm,v);
            ELdelete(lbnd);
            PQdelete(rbnd);
            ELdelete(rbnd);
            pm = le;
            if (bot->coord.y > top->coord.y) {
                temp = bot; 
                bot = top; 
                top = temp; 
                pm = re;
            }

            e = bisect(bot, top);
            bisector = HEcreate(e, pm);
            ELinsert(llbnd, bisector);

            endpoint(e, re-pm, v);

            if ((p = intersect(llbnd, bisector)) != nullptr) {
                PQdelete(llbnd);
                PQinsert(llbnd, p, dist(p,bot));
            };

            if ((p = intersect(bisector, rrbnd)) != nullptr) {
                PQinsert(bisector, p, dist(p,bot));
            };
        } else {
            break;
        }
    };

    


    for (lbnd=ELleftend->ELright; lbnd != ELrightend; lbnd=lbnd->ELright) {	
        e = lbnd -> ELedge;
        clip_line(e);
    };

    return true;
}

bool csg::scomp(VSite lhs, VSite rhs) {
    if (lhs.coord.y < rhs.coord.y) return true;
    if (lhs.coord.y > rhs.coord.y) return false;
    if (lhs.coord.x < rhs.coord.x) return true;
    if (lhs.coord.x > rhs.coord.x) return false;
    return false;
}

VSite * Voronoi::nextone() {
    if (next_site == sites.end()) {
        return nullptr;
    }
    VSite *s = &(*next_site);
    next_site ++;
    return s;
}

#pragma warning( pop )