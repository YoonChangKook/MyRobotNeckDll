#pragma once

#ifndef MY_ROBOT_NECK_DLL
#define MY_ROBOT_NECK_DLL

#ifdef MY_ROBOT_NECK_EXPORTS
#define MY_ROBOT_NECK_API __declspec(dllexport)
#else
#define MY_ROBOT_NECK_API __declspec(dllimport)
#endif

#define BUFFER_SIZE				256
#define ROT_MIN_ANGLE			0
#define DEFAULT_LISTEN_BACKLOG	4
#define INSTANCE_NAME			L"Robot Neck Control"
#ifndef PI
#define PI						3.14159265
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <thread>
#include <WinSock2.h>
#include <ws2bth.h>
#include <strsafe.h>
#include <initguid.h>
#include "CSerialPort.h"
#include "RobotVector3.h"

#pragma comment(lib,"ws2_32.lib") //Winsock Library
#pragma comment(lib, "bluetoothapis.lib")

// {2da3e5b7-6289-d59b-ab30-bf7b16cce293}
DEFINE_GUID(ROBOT_NECK_GUID, 0x2da3e5b7, 0x6289, 0xd59b, 0xab, 0x30, 0xbf, 0x7b, 0x16, 0xcc, 0xe2, 0x93);

#define NECK_DRIFT 3

enum RobotPacketType {
	CALIB = 0,
	CALIB_OK = 1,
	ROT = 2,
	ROT_OK = 3,
	PACKET_ERROR = 100
};

#pragma pack(push, 1)
typedef struct RobotPacket {
	BYTE type;
	double pitch;
	double yaw;
} RobotPacket;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct BLE_RobotPacket {
	BYTE type;
	char pitch;
	char yaw;
} BLE_RobotPacket;
#pragma pack(pop)

class MY_ROBOT_NECK_API MyRobotNeck
{
public:
	MyRobotNeck();
	virtual ~MyRobotNeck();

private:
	const double calibrate_rx = 1.25;

	WSADATA wsaData;
	bool isWsaStartup;
	// read oculus from remote pc and control robot neck (using ReadUDP function)
	std::thread* udp_reader;
	// udp socket members
	SOCKET udpSocket;
	struct sockaddr_in serverAddr;
	// read tablet and control robot neck (using ReadBluetooth function)
	std::thread* bluetooth_reader;
	// bluetooth socket members
	SOCKET bluetoothSocket;
	SOCKET bClientSocket;
	SOCKADDR_BTH bth_addr;
	WSAQUERYSET wsaQuerySet;
	CSADDR_INFO CSAddrInfo;
	wchar_t* instanceName;
	// serial port for the robot
	CSerialPort serialPort;
	bool isSerialPortConnected;
	// robot members
	double limitRXMin, limitRXMax;
	double limitRYMin, limitRYMax;
	//double last_rx, last_ry;
	RobotVector3 forward;
	double originRY;
	bool is_fix;
	// event occur when rotation value is out of limit bound.
	void(*limitEvent)(__in const double&, __in const double&);

	// thread task
	void ReadUDP();
	void ReadBluetooth();

public:
	bool StartSerialPort(__in const char* portname);
	void StopSerialPort();
	bool StartUDP(__in const short& port);
	void StopUDP();
	bool StartBluetooth();
	void StopBluetooth();
	// get limitation of x-axis, y-axis angles
	void GetLimitX(__out double& rxmin, __out double& rxmax);
	void GetLimitY(__out double& rymin, __out double& rymax);
	// Robot Method
	bool LimitInit();
	bool Calibration();
	bool Rotation(__in const double& pitch, __in const double& yaw);
	bool NoCalibrate_Rotation(__in const double& pitch, __in const double& yaw);
	// Prevent rotation from remote place (asynchronous)
	void SetRotationFix(__in const bool is_fix);
	bool GetRotationFix() const;
	void SetCurrentToOrigin();
	void GetCurrentValue(__out double& rx, __out double& ry) const;
	/*
	set event to detect whether the oculus rotation is over the robot neck rotation limit.
	occurred when the oculus rotation value is over the limit value.
	*/
	void SetLimitEvent(void(*limitEvent)(__in const double& pitch, __in const double& yaw));
};

#endif