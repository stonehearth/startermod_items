import ridl

class Point3f(ridl.Type):
   name = "csg::Point3f"
   hash = "csg::Point3f::Hash"
   key_transform = "csg::ToClosestIntTransform<3>"

class Point3(ridl.Type):
   name = "csg::Point3"
   hash = "csg::Point3::Hash"

class Point2f(ridl.Type):
   name = "csg::Point2f"
   hash = "csg::Point2::Hash"
   key_transform = "csg::ToClosestIntTransform<2>"

class Point2(ridl.Type):
   name = "csg::Point2"
   hash = "csg::Point2::Hash"

class Cube3f(ridl.Type):
   name = "csg::Cube3f"

class Cube3(ridl.Type):
   name = "csg::Cube3"

class Rect2f(ridl.Type):
   name = "csg::Rect2f"

class Rect2(ridl.Type):
   name = "csg::Rect2"

class Region3f(ridl.Type):
   name = "csg::Region3f"

class Region3(ridl.Type):
   name = "csg::Region3"

class Transform(ridl.Type):
   name = "csg::Transform"

class Quaternion(ridl.Type):
   name = "csg::Quaternion"
