#pragma once

#ifndef _ROBOT_VECTOR3_H_
#define _ROBOT_VECTOR3_H_

#ifndef ROBOT_VECTOR3_EXPORTS
#define ROBOT_VECTOR3_API __declspec(dllexport)
#else
#define ROBOT_VECTOR3_API __declspec(dllimport)
#endif

#ifndef PI
#define PI 3.14159265
#endif

#include <math.h>

struct ROBOT_VECTOR3_API RobotVector3
{
public:
	RobotVector3();
	RobotVector3(float x, float y, float z);
	virtual ~RobotVector3();

private:
	float x;
	float y;
	float z;

public:
	inline RobotVector3 operator + (const RobotVector3& other) const;
	inline RobotVector3 operator - (const RobotVector3& other) const;
	inline RobotVector3 operator * (const float& num) const;
	RobotVector3 Cross(const RobotVector3& other) const;
	float Dot(const RobotVector3& other) const;
	float Angle(const RobotVector3& other) const;
	RobotVector3 Normalize() const;
	float Distance(const RobotVector3& other) const;
	float Length() const;
};

#endif
