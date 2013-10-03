#if 0
#include "pch.h"
#include "qubicle_file.h"

using namespace ::radiant;
using namespace ::radiant::voxel;

QubicleMatrix::QubicleMatrix() :
   matrix_(nullptr)
{
}

QubicleMatrix::~QubicleMatrix()
{
   delete [] matrix_;
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

std::istream& QubicleMatrix::Read(const QbHeader& header, std::istream& in)
{
   in.read((char *)&size_.x, sizeof(uint32));
   in.read((char *)&size_.y, sizeof(uint32));
   in.read((char *)&size_.z, sizeof(uint32));

   in.read((char *)&position_.x, sizeof(uint32));
   in.read((char *)&position_.y, sizeof(uint32));
   in.read((char *)&position_.z, sizeof(uint32));

   ASSERT(header.compression == COMPRESSION_NONE);

   int len = size_.x * size_.y * size_.z;
   matrix_ = new uint32[len];
   in.read((char *)matrix_, len * sizeof(uint32));

   return in;
}

QubicleFile::QubicleFile()
{
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

      matrices_[name].Read(header, in);
   }
   return in;
}

std::istream& ::radiant::voxel::operator>>(std::istream& in, QubicleFile& q)
{
   return q.Read(in);
}
#endif