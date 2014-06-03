#include "pch.h"
#include "qubicle_file.h"

using namespace ::radiant;
using namespace ::radiant::voxel;

QubicleMatrix::QubicleMatrix(QubicleFile const& qubicle_file, std::string const& name) :
   name_(name)
{
}

QubicleMatrix::QubicleMatrix(const csg::Point3& size, const csg::Point3& position, std::string const& name) :
   size_(size), position_(position), name_(name)
{
   matrix_.resize(size.x * size.y * size.z);
}

QubicleMatrix::~QubicleMatrix()
{
}

radiant::uint32 QubicleMatrix::At(int x, int  y, int z) const
{
   ASSERT(x >= 0 && x < size_.x);
   ASSERT(y >= 0 && y < size_.y);
   ASSERT(z >= 0 && z < size_.z);

   // .qb files iterate over pixels in this direction
   //  (z (y (x))

   // Qubicle orders voxels in the file as if we were looking at the model from the
   // front.  The x coordinate we're looking for is actually on the other side.
   x = size_.x - x - 1;
   
   int offset = x + (size_.x * y) + (size_.x * size_.y * z);
   return matrix_[offset];
}

csg::Color3 QubicleMatrix::GetColor(radiant::uint32 value) const
{
   csg::Color4 c = csg::Color4::FromInteger(value);
   return csg::Color3(c.r, c.g, c.b);
}

void QubicleMatrix::Set(int x, int y, int z, radiant::uint32 value)
{
   ASSERT(x >= 0 && x < size_.x);
   ASSERT(y >= 0 && y < size_.y);
   ASSERT(z >= 0 && z < size_.z);

   x = size_.x - x - 1;  // See ::Get()

   int offset = x + (size_.x * y) + (size_.x * size_.y * z);
   matrix_[offset] = value;
}

std::istream& QubicleMatrix::Read(const QbHeader& header, CoordinateSystem coordinate_system, std::istream& in)
{
   in.read((char *)&size_.x, sizeof(uint32));
   in.read((char *)&size_.y, sizeof(uint32));
   in.read((char *)&size_.z, sizeof(uint32));

   in.read((char *)&position_.x, sizeof(uint32));
   in.read((char *)&position_.y, sizeof(uint32));
   in.read((char *)&position_.z, sizeof(uint32));

   ASSERT(header.compression == COMPRESSION_NONE);

   int len = size_.x * size_.y * size_.z;
   matrix_.resize(len);
   in.read((char *)matrix_.data(), len * sizeof(uint32));

   return in;
}

QubicleFile::QubicleFile(std::string const& uri)
{
   uri_ = uri;
}

std::istream& QubicleFile::Read(std::istream& in)
{
   char name[256];
   QbHeader header;
   in.read((char *)&header, sizeof header);

   for (uint32 i = 0; i < header.numMatricies; i++) {
      uchar c;
      in.read((char *)&c, sizeof c);
      in.read(name, c);
      name[c] = '\0';


      QubicleMatrix m(*this, name);
      m.Read(header, QubicleMatrix::CoordinateSystem::LeftHanded, in);
      matrices_.insert(std::make_pair(name, std::move(m)));
   }
   return in;
}

std::istream& ::radiant::voxel::operator>>(std::istream& in, QubicleFile& q)
{
   return q.Read(in);
}

QubicleMatrix const* QubicleFile::GetMatrix(std::string const& name) const
{
   auto i = matrices_.find(name);
   return i != matrices_.end() ? &i->second : nullptr;
}

