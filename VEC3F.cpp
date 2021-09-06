#include "VEC3F.h"

///////////////////////////////////////////////////////////////////////
// Constructors
///////////////////////////////////////////////////////////////////////
VEC3F::VEC3F(float scalar) :
  r(scalar), g(scalar), b(scalar)
{
}

VEC3F::VEC3F(float x, float y, float z) :
  r(x), g(y), b(z) 
{
}

VEC3F::VEC3F(const float v[3]) :
  r(v[0]), g(v[1]), b(v[2])  
{
}

///////////////////////////////////////////////////////////////////////
// Support functions
///////////////////////////////////////////////////////////////////////
float VEC3F::magnitude()
{ 
  return sqrt((*this) * (*this)); 
}

float VEC3F::magnitudeSq() 
{ 
  return (*this) * (*this); 
}

void VEC3F::normalize() 
{
  float l = this->magnitudeSq();
  *this /= (l < 1.0 && l > 0.0) ? sqrt(l) : 1.0;  
}

VEC3F VEC3F::fabs(const VEC3F& input)
{
  VEC3F final(input);
  final[0] = std::fabs(final[0]);
  final[1] = std::fabs(final[1]);
  final[2] = std::fabs(final[2]);

  return final;
}

///////////////////////////////////////////////////////////////////////
// Overloaded operators
///////////////////////////////////////////////////////////////////////
VEC3F& VEC3F::operator=(const VEC3F& v) 
{ 
  data[0] = v[0]; 
  data[1] = v[1]; 
  data[2] = v[2];  
  return *this; 
}

VEC3F& VEC3F::operator=(const float scalar)
{ 
  data[0] = data[1] = data[2] = scalar; 
  return *this; 
}

VEC3F& VEC3F::operator+=(const VEC3F& v)
{ 
  data[0] += v[0];
  data[1] += v[1];
  data[2] += v[2];
  return *this; 
}

VEC3F& VEC3F::operator-=(const VEC3F& v)
{ 
  data[0] -= v[0];
  data[1] -= v[1];
  data[2] -= v[2];
  return *this;
}

VEC3F& VEC3F::operator*=(float scalar)
{ 
  data[0] *= scalar;
  data[1] *= scalar;
  data[2] *= scalar;
  return *this; 
}

VEC3F& VEC3F::operator/=(float scalar)
{ 
  data[0] /= scalar;
  data[1] /= scalar;
  data[2] /= scalar;
  return *this;
}

VEC3F operator+(const VEC3F& u, const VEC3F& v)
{ 
  return VEC3F(u[0] + v[0], u[1] + v[1], u[2] + v[2]); 
}

VEC3F operator-(const VEC3F& u, const VEC3F& v)
{ 
  return VEC3F(u[0] - v[0], u[1] - v[1], u[2] - v[2]);
} 

VEC3F operator-(const VEC3F& v)
{ 
  return VEC3F(-v[0], -v[1], -v[2]);
}

float operator*(const VEC3F& u, const VEC3F& v)
{ 
  return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]; 
}

VEC3F operator*(const float scalar, const VEC3F& v)
{ 
  return VEC3F(v[0] * scalar, v[1] * scalar, v[2] * scalar);
}

VEC3F operator*(const VEC3F& v, const float scalar)
{ 
  return VEC3F(v[0] * scalar, v[1] * scalar, v[2] * scalar);
}

VEC3F operator/(const VEC3F &v, float scalar)
{ 
  return VEC3F(v[0] / scalar, v[1] / scalar, v[2] / scalar); 
}

std::ostream &operator<<(std::ostream &out, const VEC3F& v)
{ 
  return out << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")"; 
}
