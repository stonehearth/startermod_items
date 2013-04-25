#include "radiant.h"
#include "model.h"

namespace protocol = ::radiant::protocol;

using namespace ::radiant;
using ::radiant::math3d::point3;
using ::radiant::resources::Model;

Model::Model()
{
   _offset.set_zero();
}

Model::Model(std::string mesh, std::string texture, std::string material, const math3d::point3 &offset) :
   _mesh(mesh),
   _texture(texture),
   _material(material),
   _offset(offset)
{
}

void Model::SetMesh(std::string mesh)
{
   _mesh = mesh;
}

void Model::SetTexture(std::string texture)
{
   _texture = texture;
}

void Model::SetMaterial(std::string material)
{
   _material = material;
}

void Model::SetOffset(const point3 &offset)
{
   _offset = offset;
}

std::string Model::GetMesh() const
{
   return _mesh;
}

std::string Model::GetTexture() const
{
   return _texture;
}

std::string Model::GetMaterial() const
{
   return _material;
}

point3 Model::GetOffset() const
{
   return _offset;
}
