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

#ifndef RADIANT_CSG_VORONI_H
#define RADIANT_CSG_VORONI_H

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <list>
#include "point.h"
#include "namespace.h"

BEGIN_RADIANT_CSG_NAMESPACE

class VSitePoints {
public:
   VSitePoints();
   VSitePoints(int x0, int y0, int x1, int y1) : p0(x0, y0), p1(x1, y1) { }

   csg::Point2 p0, p1;
};

class VSite	{
public:
    Point2f coord;
    std::vector<VSitePoints> edges;
};

class VEdge {
public:
    float   a,b,c;
    VSite *ep[2];
    VSite *reg[2];

    VEdge():a(0),b(0),c(0) {
        ep[0] = ep[1] = nullptr;
        reg[0] = reg[1] = nullptr;
    }
};

class Halfedge {
public:
    bool deleted;
    Halfedge *ELleft, *ELright;
    VEdge *ELedge;
    char ELpm;
    VSite *vertex;
    float ystar;
    Halfedge *PQnext;

    Halfedge():deleted(false),ELleft(nullptr), ELright(nullptr),
        ELedge(nullptr), ELpm(0), vertex(nullptr), ystar(0),
        PQnext(nullptr) {};

    Halfedge(VEdge *e, int pm):deleted(false),ELleft(nullptr),
        ELright(nullptr), ELedge(e), ELpm(pm), vertex(nullptr),
        ystar(0), PQnext(nullptr) {};
};


class Voronoi {
public:
    Voronoi(std::vector<Point2> &points,
            float minX, float maxX,
            float minY, float maxY);

    std::list<VSite> sites;

private:
    int nsites;

    std::list<Halfedge> hf1;
    std::list<VSite> sf1;
    std::list<VEdge> ef1;
    
    std::vector<Halfedge*> ELhash;
    std::vector<Halfedge> PQhash;
    int PQcount;
    int PQmin;

    Halfedge *ELleftend, *ELrightend;
    float xmin, xmax, ymin, ymax, deltax, deltay;
    std::list<VSite>::iterator next_site;
    VSite *bottomsite;
    float borderMinX, borderMaxX, borderMinY, borderMaxY;


    Halfedge *HEcreate(VEdge *e,int pm);
    void ELdelete(Halfedge *he);


    int PQempty();
    Point2f PQ_min();
    Halfedge *PQextractmin();	
    bool voronoi();
    void endpoint(VEdge *e, int lr, VSite * s);

    Halfedge *ELleftbnd(Point2f *p);

    void PQinsert(Halfedge *he,VSite * v, float offset);
    void PQdelete(Halfedge *he);
    void ELinsert(Halfedge *lb, Halfedge *newHe);
    Halfedge *ELgethash(int b);
    Halfedge *ELleft(Halfedge *he);
    VSite *leftreg(Halfedge *he);
    int PQbucket(Halfedge *he);
    void clip_line(VEdge *e);
    int right_of(Halfedge *el, Point2f *p);

    VSite *rightreg(Halfedge *he);
    VEdge *bisect(VSite *s1,VSite *s2);
    float dist(VSite *s,VSite *t);
    VSite *intersect(Halfedge *el1, Halfedge *el2, Point2f *p=0);

    VSite *nextone();
};

bool scomp(VSite, VSite);

END_RADIANT_CSG_NAMESPACE

#endif

