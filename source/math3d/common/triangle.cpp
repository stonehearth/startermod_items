#include "radiant.h"
#include "radiant.pb.h"
#include "common.h"

using namespace radiant;
using namespace radiant::math3d;

//----------------------------------------------------------------------------
// @ IvTriangle.cpp
// 
// Helpful routines for triangle data
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//----------------------------------------------------------------------------
//-- Includes ----------------------------------------------------------------
//----------------------------------------------------------------------------

#include "triangle.h"
#include "point3.h"
#include "point2.h"
#include "ray3.h"
#include "plane.h"

//----------------------------------------------------------------------------
//-- Functions ---------------------------------------------------------------
//----------------------------------------------------------------------------

// helper functions
inline bool adjust_q(const point3& p0, const point3& p1, 
                     const point3& p2, const point3& q0, 
                     const point3& q1, const point3& q2,
                     float test_q0, float test_q1, float test_q2,
                     const vector3& normal_p);

inline bool test_line_overlap(const point3& p0, const point3& p1, 
                     const point3& p2, const point3& q0, 
                     const point3& q1, const point3& q2);

inline bool colpanar_triangle_intersect(const point3& p0, 
                     const point3& p1, const point3& p2, 
                     const point3& q0, const point3& q1, 
                     const point3& q2, const vector3& plane_normal);

inline bool edge_test(const point2& edge_point, const point2& edge_vector, 
                      float n, const point2& p0, const point2& p1, const point2& p2);

//-------------------------------------------------------------------------------
// @ ::is_point_in_triangle()
//-------------------------------------------------------------------------------
// Returns true if point on triangle plane lies inside triangle (3D version)
// Assumes triangle is not degenerate
//-------------------------------------------------------------------------------
bool is_point_in_triangle(const point3& point, const point3& p0,
                          const point3& p1, const point3& p2)
{
    vector3 v0 = vector3(p1 - p0);
    vector3 v1 = vector3(p2 - p1);
    vector3 n = v0.cross(v1);

    vector3 w_test = v0.cross(vector3(point - p0));
    if (w_test.dot(n) < 0.0f)
    {
        return false;
    }

    w_test = v1.cross(vector3(point - p1));
    if (w_test.dot(n) < 0.0f)
    {
        return false;
    }

    vector3 v2 = vector3(p0 - p2);
    w_test = v2.cross(vector3(point - p2));
    if (w_test.dot(n) < 0.0f)
    {
        return false;
    }

    return true;
}


//-------------------------------------------------------------------------------
// @ ::barycentric_coordinates()
//-------------------------------------------------------------------------------
// Returns barycentric coordinates for point inside triangle (3D version)
// Assumes triangle is not degenerate
//-------------------------------------------------------------------------------
void barycentric_coordinates(float &r, float &s, float& t, 
                             const point3& point, const point3& p0,
                             const point3& p1, const point3& p2)
{
    // get difference vectors
    vector3 u = vector3(p1 - p0);
    vector3 v = vector3(p2 - p0);
    vector3 w = vector3(point - p0);

    // compute cross product to get area of parallelograms
    vector3 a = u.cross(w);
    vector3 b = v.cross(w);
    vector3 c = u.cross(v);
    
    // compute barycentric coordinates as ratios of areas
    float denom = 1.0f / c.length();
    s = b.length() * denom;
    t = a.length() * denom;
    r = 1.0f - s - t;
}


//-------------------------------------------------------------------------------
// @ ::triangle_intersect()
//-------------------------------------------------------------------------------
// Returns true if triangles P0P1P2 and Q0Q1Q2 intersect
// Assumes triangle is not degenerate
//
// This is not the algorithm presented in the text.  Instead, it is based on a 
// recent article by Guigue and Devillers in the July 2003 issue Journal of 
// Graphics Tools.  As it is faster than the ERIT algorithm, under ordinary 
// circumstances it would have been discussed in the text, but it arrived too late.  
//
// More information and the original source code can be found at
// http://www.acm.org/jgt/papers/GuigueDevillers03/
//
// A nearly identical algorithm was in the same issue of JGT, by Shen Heng and 
// Tang.  See http://www.acm.org/jgt/papers/ShenHengTang03/ for source code.
//
// Yes, this is complicated.  Did you think testing triangles would be easy?
//-------------------------------------------------------------------------------
bool ::radiant::math3d::triangle_intersect(const point3& p0, const point3& p1, 
                        const point3& p2, const point3& q0, 
                        const point3& q1, const point3& q2)
{
    // test P against Q's plane
    vector3 normalQ = vector3(q1 - q0).cross(vector3(q2 - q0));

    float test_p0 = normalQ.dot(vector3(p0 - q0));
    float test_p1 = normalQ.dot(vector3(p1 - q0));
    float test_p2 = normalQ.dot(vector3(p2 - q0));
  
    // P doesn't intersect Q's plane
    if (test_p0 * test_p1 > k_epsilon && test_p0 * test_p2 > k_epsilon)
        return false;

    // test Q against P's plane
    vector3 normal_p = vector3(p1 - p0).cross(vector3(p2 - p0));

    float test_q0 = normal_p.dot(vector3(q0 - p0));
    float test_q1 = normal_p.dot(vector3(q1 - p0));
    float test_q2 = normal_p.dot(vector3(q2 - p0));
  
    // Q doesn't intersect P's plane
    if (test_q0 * test_q1 > k_epsilon && test_q0 * test_q2 > k_epsilon)
        return false;
    
    // now we rearrange P's vertices such that the lone vertex (the one that lies
    // in its own half-space of Q) is first.  We also permute the other
    // triangle's vertices so that p0 will "see" them in counterclockwise order

    // Once reordered, we pass the vertices down to a helper function which will
    // reorder Q's vertices, and then test

    // p0 in Q's positive half-space
    if (test_p0 > k_epsilon) 
    {
        // p1 in Q's positive half-space (so p2 is lone vertex)
        if (test_p1 > k_epsilon) 
            return adjust_q(p2, p0, p1, q0, q2, q1, test_q0, test_q2, test_q1, normal_p);
        // p2 in Q's positive half-space (so p1 is lone vertex)
        else if (test_p2 > k_epsilon) 
            return adjust_q(p1, p2, p0, q0, q2, q1, test_q0, test_q2, test_q1, normal_p);	
        // p0 is lone vertex
        else 
            return adjust_q(p0, p1, p2, q0, q1, q2, test_q0, test_q1, test_q2, normal_p);
    } 
    // p0 in Q's negative half-space
    else if (test_p0 < -k_epsilon) 
    {
        // p1 in Q's negative half-space (so p2 is lone vertex)
        if (test_p1 < -k_epsilon) 
            return adjust_q(p2, p0, p1, q0, q1, q2, test_q0, test_q1, test_q2, normal_p);
        // p2 in Q's negative half-space (so p1 is lone vertex)
        else if (test_p2 < -k_epsilon) 
            return adjust_q(p1, p2, p0, q0, q1, q2, test_q0, test_q1, test_q2, normal_p);
        // p0 is lone vertex
        else 
            return adjust_q(p0, p1, p2, q0, q2, q1, test_q0, test_q2, test_q1, normal_p);
    } 
    // p0 on Q's plane
    else 
    {
        // p1 in Q's negative half-space 
        if (test_p1 < -k_epsilon) 
        {
            // p2 in Q's negative half-space (p0 is lone vertex)
            if (test_p2 < -k_epsilon) 
                return adjust_q(p0, p1, p2, q0, q1, q2, test_q0, test_q1, test_q2, normal_p);
            // p2 in positive half-space or on plane (p1 is lone vertex)
            else 
                return adjust_q(p1, p2, p0, q0, q2, q1, test_q0, test_q2, test_q1, normal_p);
        }
        // p1 in Q's positive half-space 
        else if (test_p1 > k_epsilon) 
        {
            // p2 in Q's positive half-space (p0 is lone vertex)
            if (test_p2 > k_epsilon) 
                return adjust_q(p0, p1, p2, q0, q2, q1, test_q0, test_q2, test_q1, normal_p);
            // p2 in negative half-space or on plane (p1 is lone vertex)
            else 
                return adjust_q(p1, p2, p0, q0, q1, q2, test_q0, test_q1, test_q2, normal_p);
        }
        // p1 lies on Q's plane too
        else  
        {
            // p2 in Q's positive half-space (p2 is lone vertex)
            if (test_p2 > k_epsilon) 
                return adjust_q(p2, p0, p1, q0, q1, q2, test_q0, test_q1, test_q2, normal_p);
            // p2 in Q's negative half-space (p2 is lone vertex)
            // note different ordering for Q vertices
            else if (test_p2 < -k_epsilon) 
                return adjust_q(p2, p0, p1, q0, q2, q1, test_q0, test_q2, test_q1, normal_p);
            // all three points lie on Q's plane, default to 2D test
            else 
                return colpanar_triangle_intersect(p0, p1, p2, q0, q1, q2, normal_p);
        }
    }
}


//-------------------------------------------------------------------------------
// @ ::adjust_q()
//-------------------------------------------------------------------------------
// Helper for triangle_intersect()
//
// Now we rearrange Q's vertices such that the lone vertex (the one that lies
// in its own half-space of P) is first.  We also permute the other
// triangle's vertices so that q0 will "see" them in counterclockwise order
//
// Once reordered, we pass the vertices down to a helper function which will
// actually test for intersection on the common line between the two planes
//-------------------------------------------------------------------------------
inline bool 
adjust_q(const point3& p0, const point3& p1, 
                   const point3& p2, const point3& q0, 
                   const point3& q1, const point3& q2,
                   float test_q0, float test_q1, float test_q2,
                   const vector3& normal_p)
{

    // q0 in P's positive half-space
    if (test_q0 > k_epsilon) 
    { 
        // q1 in P's positive half-space (so q2 is lone vertex)
        if (test_q1 > k_epsilon) 
            return test_line_overlap(p0, p2, p1, q2, q0, q1);
        // q2 in P's positive half-space (so q1 is lone vertex)
        else if (test_q2 > k_epsilon) 
            return test_line_overlap(p0, p2, p1, q1, q2, q0);
        // q0 is lone vertex
        else 
            return test_line_overlap(p0, p1, p2, q0, q1, q2);
    }
    // q0 in P's negative half-space
    else if (test_q0 < -k_epsilon) 
    { 
        // q1 in P's negative half-space (so q2 is lone vertex)
        if (test_q1 < -k_epsilon) 
            return test_line_overlap(p0, p1, p2, q2, q0, q1);
        // q2 in P's negative half-space (so q1 is lone vertex)
        else if (test_q2 < -k_epsilon) 
            return test_line_overlap(p0, p1, p2, q1, q2, q0);
        // q0 is lone vertex
        else 
            return test_line_overlap(p0, p2, p1, q0, q1, q2);
    }
    // q0 on P's plane
    else 
    { 
        // q1 in P's negative half-space 
        if (test_q1 < -k_epsilon) 
        { 
            // q2 in P's negative half-space (q0 is lone vertex)
            if (test_q2 < -k_epsilon)  
                return test_line_overlap(p0, p1, p2, q0, q1, q2);
            // q2 in positive half-space or on plane (p1 is lone vertex)
            else 
                return test_line_overlap(p0, p2, p1, q1, q2, q0);
        }
        // q1 in P's positive half-space 
        else if (test_q1 > k_epsilon) 
        { 
            // q2 in P's positive half-space (q0 is lone vertex)
            if (test_q2 > k_epsilon) 
                return test_line_overlap(p0, p2, p1, q0, q1, q2);
            // q2 in negative half-space or on plane (p1 is lone vertex)
            else  
                return test_line_overlap(p0, p1, p2, q1, q2, q0);
        }
        // q1 lies on P's plane too
        else 
        {
            // q2 in P's positive half-space (q2 is lone vertex)
            if (test_q2 > k_epsilon) 
                return test_line_overlap(p0, p1, p2, q2, q0, q1);
            // q2 in P's negative half-space (q2 is lone vertex)
            // note different ordering for Q vertices
            else if (test_q2 < -k_epsilon) 
                return test_line_overlap(p0, p2, p1, q2, q0, q1);
            // all three points lie on P's plane, default to 2D test
            else 
                return colpanar_triangle_intersect(p0, p1, p2, q0, q1, q2, normal_p);
        }
    }
}


//-------------------------------------------------------------------------------
// @ ::test_line_overlap()
//-------------------------------------------------------------------------------
// Helper for triangle_intersect()
//
// This tests whether the rearranged triangles overlap, by checking the intervals
// where their edges cross the common line between the two planes.  If the 
// interval for P is [i,j] and Q is [k,l], then there is intersection if the
// intervals overlap.  Previous algorithms computed these intervals directly, 
// this tests implictly by using two "plane tests."
//-------------------------------------------------------------------------------
inline bool 
test_line_overlap(const point3& p0, const point3& p1, 
                   const point3& p2, const point3& q0, 
                   const point3& q1, const point3& q2)
{
    // get "plane normal"
    vector3 normal = vector3(p1-p0).cross(vector3(q0-p0));

    // fails test, no intersection
    if (normal.dot(vector3(q1 - p0)) > k_epsilon)
        return false;
  
    // get "plane normal"
    normal = vector3(p2-p0).cross(vector3(q2-p0));

    // fails test, no intersection
    if (normal.dot(vector3(q0 - p0)) > k_epsilon)
        return false;

    // intersection!
    return true;
}


//-------------------------------------------------------------------------------
// @ ::colpanar_triangle_intersect()
//-------------------------------------------------------------------------------
// Helper for triangle_intersect()
//
// This projects the two triangles down to 2D, maintaining the largest area by
// dropping the dimension where the normal points the farthest.
//-------------------------------------------------------------------------------
inline bool colpanar_triangle_intersect(const point3& p0, 
                     const point3& p1, const point3& p2, 
                     const point3& q0, const point3& q1, 
                     const point3& q2, const vector3& plane_normal)
{
    point3 abs_normal(math3d::abs(plane_normal.x), math3d::abs(plane_normal.y), 
                       math3d::abs(plane_normal.z));

    point2 proj_p0, proj_p1, proj_p2;
    point2 proj_q0, proj_q1, proj_q2;

    // if x is direction of largest magnitude
    if (abs_normal.x > abs_normal.y && abs_normal.x >= abs_normal.z)
    {
        proj_p0.set(p0.y, p0.z);
        proj_p1.set(p1.y, p1.z);
        proj_p2.set(p2.y, p2.z);
        proj_q0.set(q0.y, q0.z);
        proj_q1.set(q1.y, q1.z);
        proj_q2.set(q2.y, q2.z);
    }
    // if y is direction of largest magnitude
    else if (abs_normal.y > abs_normal.x && abs_normal.y >= abs_normal.z)
    {
        proj_p0.set(p0.x, p0.z);
        proj_p1.set(p1.x, p1.z);
        proj_p2.set(p2.x, p2.z);
        proj_q0.set(q0.x, q0.z);
        proj_q1.set(q1.x, q1.z);
        proj_q2.set(q2.x, q2.z);
    }
    // z is the direction of largest magnitude
    else
    {
        proj_p0.set(p0.x, p0.y);
        proj_p1.set(p1.x, p1.y);
        proj_p2.set(p2.x, p2.y);
        proj_q0.set(q0.x, q0.y);
        proj_q1.set(q1.x, q1.y);
        proj_q2.set(q2.x, q2.y);
    }

    return triangle_intersect(proj_p0, proj_p1, proj_p2, proj_q0, proj_q1, proj_q2);
}



//-------------------------------------------------------------------------------
// @ ::triangle_intersect()
//-------------------------------------------------------------------------------
// Returns true if ray intersects triangle
//-------------------------------------------------------------------------------
bool
::radiant::math3d::triangle_intersect(float& t, const point3& p0, const point3& p1, 
                   const point3& p2, const ray3& ray)
{
    // test ray direction against triangle
    vector3 e1 = vector3(p1 - p0);
    vector3 e2 = vector3(p2 - p0);
    vector3 p = ray.direction.cross(e2);
    float a = e1.dot(p);

    // if result zero, no intersection or infinite intersections
    // (ray parallel to triangle plane)
    if (math3d::is_zero(a))
        return false;

    // compute denominator
    float f = 1.0f/a;

    // compute barycentric coordinates
    vector3 s = vector3(ray.origin - p0);
    float u = f * s.dot(p);

    // ray falls outside triangle
    if (u < 0.0f || u > 1.0f) 
        return false;

    vector3 q = s.cross(e1);
    float v = f * ray.direction.dot(q);

    // ray falls outside triangle
    if (v < 0.0f || u+v > 1.0f) 
        return false;

    // compute line parameter
    t = f * e2.dot(q);

    return (t >= 0.0f);
}


//-------------------------------------------------------------------------------
// @ ::triangle_classify()
//-------------------------------------------------------------------------------
// Returns signed distance between plane and triangle
//-------------------------------------------------------------------------------
float
triangle_classify(const point3& p0, const point3& p1, 
                  const point3& p2, const plane& plane)
{
    float test0 = plane.test(p0);
    float test1 = plane.test(p1);

    // if two points lie on opposite sides of plane, intersect
    if (test0*test1 < 0.0f)
        return 0.0f;

    float test2 = plane.test(p2);

    // if two points lie on opposite sides of plane, intersect
    if (test0*test2 < 0.0f)
        return 0.0f;
    if (test1*test2 < 0.0f)
        return 0.0f;

    // no intersection, return signed distance
    if (test0 < 0.0f)
    {
        if (test0 < test1)
        {
            if (test1 < test2)
                return test2;
            else
                return test1;
        }
        else if (test0 < test2)
        {
            return test2;
        }
        else
        {   
            return test0;
        }
    }
    else
    {
        if (test0 > test1)
        {
            if (test1 > test2)
                return test2;
            else
                return test1;
        }
        else if (test0 > test2)
        {
            return test2;
        }
        else
        {   
            return test0;
        }
    }
    
}


//-------------------------------------------------------------------------------
// @ ::is_point_in_triangle()
//-------------------------------------------------------------------------------
// Returns true if point lies inside triangle (2D version)
// Assumes triangle is not degenerate
//-------------------------------------------------------------------------------
bool is_point_in_triangle(const point2& point, const point2& p0,
                          const point2& p1, const point2& p2)
{
    point2 v0 = p1 - p0;
    point2 v1 = p2 - p1;
    float n = v0.perp_dot(v1);

    float w_test = v0.perp_dot(point - p0);
    if (w_test*n < 0.0f)
    {
        return false;
    }

    w_test = v1.perp_dot(point - p1);
    if (w_test*n < 0.0f)
    {
        return false;
    }

    point2 v2 = p0 - p2;
    w_test = v2.perp_dot(point - p2);
    if (w_test*n < 0.0f)
    {
        return false;
    }

    return true;

}   // End of is_point_in_triangle()


//-------------------------------------------------------------------------------
// @ ::barycentric_coordinates()
//-------------------------------------------------------------------------------
// Returns barycentric coordinates for point inside triangle (2D version)
// Assumes triangle is not degenerate
//-------------------------------------------------------------------------------
void barycentric_coordinates(float &r, float &s, float& t, 
                             const point2& point, const point2& p0,
                             const point2& p1, const point2& p2)
{
    // get difference vectors
    point2 u = p1 - p0;
    point2 v = p2 - p0;
    point2 w = point - p0;

    // compute perpendicular dot product to get area of parallelograms
    float a = u.perp_dot(w);
    float b = v.perp_dot(w);
    float c = u.perp_dot(v);
    
    // compute barycentric coordinates as ratios of areas
    float denom = 1.0f/c;
    s = b*denom;
    t = a*denom;
    r = 1.0f - s - t;

}   // End of barycentric_coordinates()


//-------------------------------------------------------------------------------
// @ ::triangle_intersect()
//-------------------------------------------------------------------------------
// Returns true if triangles P0P1P2 and Q0Q1Q2 intersect (2D version)
// Assumes triangle is not degenerate
//
// Uses principle of separating planes again: if all three points of one triangle
// lie outside one edge of the other, then they are disjoint
//
// This could probably be made faster.  
// See // http://www.acm.org/jgt/papers/GuigueDevillers03/ for another possible
// implementation.
//-------------------------------------------------------------------------------
bool ::radiant::math3d::triangle_intersect(const point2& p0, const point2& p1, 
                        const point2& p2, const point2& q0, 
                        const point2& q1, const point2& q2)
{
    // test Q against P
    point2 v0 = p1 - p0;
    point2 v1 = p2 - p1;
    float n = v0.perp_dot(v1);

    // test against edge 0
    if (edge_test(p0, v0, n, q0, q1, q2))
        return false;

    // test against edge 1
    if (edge_test(p1, v1, n, q0, q1, q2))
        return false;

    // test against edge 2
    point2 v2 = p0 - p2;
    if (edge_test(p2, v2, n, q0, q1, q2))
        return false;

    // test P against Q
    v0 = q1 - q0;
    v1 = q2 - q1;
    n = v0.perp_dot(v1);

    // test against edge 0
    if (edge_test(q0, v0, n, p0, p1, p2))
        return false;

    // test against edge 1
    if (edge_test(q1, v1, n, p0, p1, p2))
        return false;

    // test against edge 2
    v2 = q0 - q2;
    if (edge_test(q2, v2, n, p0, p1, p2))
        return false;

    return true;
}


//-------------------------------------------------------------------------------
// @ ::edge_test()
//-------------------------------------------------------------------------------
// Helper function for 2D triangle_intersect()
//
// Given an edge vector and origin, triangle "normal" value, and three points,
// determine if all three points lie outside the edge
//-------------------------------------------------------------------------------
inline bool 
edge_test(const point2& edge_point, const point2& edge_vector, float n, 
          const point2& p0, const point2& p1, const point2& p2)
{
    // test each point against the edge in turn
    float w_test = edge_vector.perp_dot(p0 - edge_point);
    if (w_test*n > 0.0f)
    {
        return false;
    }
    w_test = edge_vector.perp_dot(p1 - edge_point);
    if (w_test*n > 0.0f)
    {
        return false;
    }
    w_test = edge_vector.perp_dot(p2 - edge_point);
    if (w_test*n > 0.0f)
    {
        return false;
    }

    return true;
}
