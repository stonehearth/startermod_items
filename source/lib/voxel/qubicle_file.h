#ifndef _RADIANT_VOXEL_QUBICLE_FILE_H
#define _RADIANT_VOXEL_QUBICLE_FILE_H

#include "namespace.h"
#include "csg/point.h"
#include "csg/color.h"
#include "qubicle_file_format.h"
#include <unordered_map>

BEGIN_RADIANT_VOXEL_NAMESPACE

class QubicleMatrix {
public:
   QubicleMatrix(const csg::Point3& size, const csg::Point3& position, const std::string& name);
   ~QubicleMatrix();

   std::string const& GetName() const { return name_; }
   std::istream& Read(QbHeader const& header, std::istream& in);  
   const csg::Point3& GetSize() const { return size_; }
   const csg::Point3& GetPosition() const { return position_; }

   uint32 At(int x, int  y, int z) const;
   csg::Color3 GetColor(uint32 value) const;
   void Set(int x, int y, int z, uint32);

private:
   friend QubicleFile;
   QubicleMatrix(QubicleFile const& qubicle_file, std::string const& name);

private:
   csg::Point3          size_;
   csg::Point3          position_;
   std::vector<uint32>  matrix_;
   std::string          name_;
};

class QubicleFile {
public:
   QubicleFile(std::string const& uri);

   std::string const& GetUri() const { return uri_; }
   std::istream& Read(std::istream& in);
   QubicleMatrix* GetMatrix(std::string const& name);

   typedef std::unordered_map<std::string, QubicleMatrix> MatrixMap;

   const MatrixMap::const_iterator begin() const { return matrices_.begin(); }
   const MatrixMap::const_iterator end() const { return matrices_.end(); }

private:
   std::string uri_; 
   MatrixMap matrices_;
};

std::istream& operator>>(std::istream& in, QubicleMatrix& q);
std::istream& operator>>(std::istream& in, QubicleFile& q);

END_RADIANT_VOXEL_NAMESPACE


#endif //  _RADIANT_VOXEL_QUBICLE_FILE_H
