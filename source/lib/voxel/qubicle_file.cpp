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
   // Left handed .qb files iterate over pixels in
   // this direction
   //  (z (y (x))

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
   int offset = x + (size_.x * y) + (size_.x * size_.y * z);
   matrix_[offset] = value;
}

// Qubicle Constructor determines the model origin by taking the center of the bounding box of the matrix.
// To ensure the origin is correct, make sure the bounding box is centered about the y axis.
// For bounding boxes with even widths/lengths the y axis will poke out of the center of the model.
// For bounding boxes with odd widths/lengths, the center voxel should be in the positive x,z quadrant of the y axis.
// It helps a lot to get a tight bounding box by using the Modify->Optimize Size command.
// Note that Qubicle Constructor does not preseve the same model center when exporting to left handed coordinate system
// when the bounding box has an odd length. This is corrected in the code below.
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

   if (coordinate_system == CoordinateSystem::LeftHanded) {
      // Correct for rounding error when Qubicle Constructor translates and inverts the z-axis
      // for left handed coordinate system during export
      position_.z += size_.z % 2;
   }

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

QubicleMatrix* QubicleFile::GetMatrix(std::string const& name)
{
   auto i = matrices_.find(name);
   return i != matrices_.end() ? &i->second : nullptr;
}

