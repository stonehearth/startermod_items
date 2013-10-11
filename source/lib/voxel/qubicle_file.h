#ifndef _RADIANT_VOXEL_QUBICLE_FILE_H
#define _RADIANT_VOXEL_QUBICLE_FILE_H

#include "namespace.h"
#include "csg/point.h"
#include "csg/color.h"
#include "qubicle_file_format.h"

BEGIN_RADIANT_VOXEL_NAMESPACE

class QubicleMatrix {
public:
   QubicleMatrix();
   ~QubicleMatrix();

   std::istream& Read(const QbHeader& header, std::istream& in);  
   const csg::Point3& GetSize() const { return size_; }
   const csg::Point3& GetPosition() const { return position_; }

   uint32 At(int x, int  y, int z) const;
   csg::Color3 GetColor(uint32 value) const;

public:
   csg::Point3    size_;
   csg::Point3    position_;
   uint32*        matrix_;
};

class QubicleFile {
public:
   QubicleFile();

   std::istream& Read(std::istream& in);

   typedef std::map<std::string, QubicleMatrix> MatrixMap;

   const MatrixMap::const_iterator begin() const { return matrices_.begin(); }
   const MatrixMap::const_iterator end() const { return matrices_.end(); }

protected:
   MatrixMap matrices_;
};

std::istream& operator>>(std::istream& in, QubicleMatrix& q);
std::istream& operator>>(std::istream& in, QubicleFile& q);

END_RADIANT_VOXEL_NAMESPACE


#endif //  _RADIANT_VOXEL_QUBICLE_FILE_H
