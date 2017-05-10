#include "stdafx.h"
#include "RobotVector3.h"

RobotVector3::RobotVector3()
	: x(0), y(0), z(0)
{}

RobotVector3::RobotVector3(float x, float y, float z)
	: x(x), y(y), z(z)
{}

RobotVector3::~RobotVector3()
{}

RobotVector3 RobotVector3::operator+(const RobotVector3& other) const
{
	return RobotVector3(x + other.x, y + other.y, z + other.z);
}

RobotVector3 RobotVector3::operator-(const RobotVector3& other) const
{
	return RobotVector3(x - other.x, y - other.y, z - other.z);
}

RobotVector3 RobotVector3::operator*(const float& num) const
{
	return RobotVector3(x * num, y * num, z * num);
}

RobotVector3 RobotVector3::Cross(const RobotVector3& other) const
{
	return RobotVector3(y * other.z - z * other.y, 
						z * other.x - x * other.z,
						x * other.y - y * other.x);
}

float RobotVector3::Dot(const RobotVector3& other) const
{
	return x * other.x + y * other.y + z * other.z;
}

float RobotVector3::Angle(const RobotVector3& other) const
{
	RobotVector3 nThis = Normalize();
	RobotVector3 nOther = other.Normalize();
	float radian = acos(nThis.Dot(nOther));
	return 180 * radian / PI;
}

RobotVector3 RobotVector3::Normalize() const
{
	float length = Length();
	return RobotVector3(x / length, y / length, z / length);
}

float RobotVector3::Distance(const RobotVector3& other) const
{
	float dx = x - other.x;
	float dy = y - other.y;
	float dz = z - other.z;
	return sqrt(dx * dx + dy * dy + dz * dz);
}

float RobotVector3::Length() const
{
	return sqrt((x * x) + (y * y) + (z * z));
}

float RobotVector3::GetX() const { return this->x; }
float RobotVector3::GetY() const { return this->y; }
float RobotVector3::GetZ() const { return this->z; }