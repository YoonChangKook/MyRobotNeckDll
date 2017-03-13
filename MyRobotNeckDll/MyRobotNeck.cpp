// MyRobotNeckDll.cpp : DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"
#include "MyRobotNeck.h"

using std::thread;

MyRobotNeck::MyRobotNeck()
	: wsaData({ 0 }), isWsaStartup(false),
	udpSocket(INVALID_SOCKET), udp_reader(NULL), 
	bluetoothSocket(INVALID_SOCKET), bClientSocket(INVALID_SOCKET), bluetooth_reader(NULL),
	wsaQuerySet({ 0 }), CSAddrInfo({ 0 }), instanceName(NULL), 
	isSerialPortConnected(false), 
	limitRXMin(0), limitRXMax(0), limitRYMin(0), limitRYMax(0), forward(), limitEvent(NULL)
{
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		ZeroMemory(&wsaData, sizeof(wsaData));
		return;
	}
	else
		isWsaStartup = true;
}

MyRobotNeck::~MyRobotNeck()
{
	if(isSerialPortConnected)
		this->serialPort.ClosePort();

	// close and release udp socket
	if (this->udpSocket != INVALID_SOCKET)
		closesocket(this->udpSocket);
	if (this->udp_reader != NULL)
	{
		this->udp_reader->detach();
		delete this->udp_reader;
	}
	// close and release bluetooth socket
	if (this->bluetoothSocket != INVALID_SOCKET)
		closesocket(this->bluetoothSocket);
	if (this->bluetooth_reader != NULL)
	{
		this->bluetooth_reader->detach();
		delete this->bluetooth_reader;
	}
}

bool MyRobotNeck::StartSerialPort(__in const char* portname)
{
	if (this->serialPort.OpenPort(portname))
	{
		if (!serialPort.Configure(CBR_115200, 8, FALSE, NOPARITY, ONESTOPBIT))
		{
			this->serialPort.ClosePort();
			return false;
		}

		if (!serialPort.SetCommunicationTimeouts(0, 0, 500, 0, 500))
		{
			this->serialPort.ClosePort();
			return false;
		}

		this->isSerialPortConnected = true;

		return true;
	}
	else
		return false;
}

void MyRobotNeck::StopSerialPort()
{
	this->serialPort.ClosePort();
	this->isSerialPortConnected = false;
}

bool MyRobotNeck::StartUDP(__in const short& port)
{
	// check wsa
	if (isWsaStartup == false)
	{
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			return false;
		else
			isWsaStartup = true;
	}

	// close socket and reassign
	StopUDP();

	if ((this->udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
		return false;

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);

	if (bind(this->udpSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return false;

	// start thread
	this->udp_reader = new thread(&MyRobotNeck::ReadUDP ,this);

	return true;
}

void MyRobotNeck::StopUDP()
{
	if (this->udpSocket != INVALID_SOCKET)
	{
		closesocket(this->udpSocket);
		this->udpSocket = INVALID_SOCKET;
	}
	if (this->udp_reader != NULL)
	{
		this->udp_reader->detach();
		delete this->udp_reader;
		this->udp_reader = NULL;
	}
}

bool MyRobotNeck::StartBluetooth()
{
	// check wsa
	if (isWsaStartup == false)
	{
		if (WSAStartup(MAKEWORD(2, 2), &wsaData))
		{
			//printf("WSA Startup Fail\n");
			return false;
		}
		else
			isWsaStartup = true;
	}

	// close socket and reassign
	StopBluetooth();

	ZeroMemory(&this->bth_addr, sizeof(SOCKADDR_BTH));
	ZeroMemory(&this->CSAddrInfo, sizeof(CSADDR_INFO));
	ZeroMemory(&this->wsaQuerySet, sizeof(WSAQUERYSET));
	int bth_addr_len = sizeof(SOCKADDR_BTH);
	wchar_t comName[16];
	DWORD lenComName = 16;

	// get computer name
	if (!GetComputerName(comName, &lenComName))
	{
		//printf("Get Computer Name Fail\n");
		return false;
	}

	// make socket
	if ((this->bluetoothSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)) == INVALID_SOCKET)
	{
		//printf("Socket Fail");
		return false;
	}

	bth_addr.addressFamily = AF_BTH;
	bth_addr.port = BT_PORT_ANY;

	// bind socket
	if (bind(this->bluetoothSocket, (struct sockaddr*)&bth_addr, sizeof(SOCKADDR_BTH)) == SOCKET_ERROR)
	{
		//printf("Bind Fail\n");
		return false;
	}

	// get socket name
	if (getsockname(bluetoothSocket, (struct sockaddr *)&bth_addr, &bth_addr_len) == SOCKET_ERROR)
	{
		//printf("Get Socket Name Fail\n");
		return false;
	}

	// set CSADDRInfo
	CSAddrInfo.LocalAddr.iSockaddrLength = sizeof(SOCKADDR_BTH);
	CSAddrInfo.LocalAddr.lpSockaddr = (LPSOCKADDR)&bth_addr;
	CSAddrInfo.RemoteAddr.iSockaddrLength = sizeof(SOCKADDR_BTH);
	CSAddrInfo.RemoteAddr.lpSockaddr = (LPSOCKADDR)&bth_addr;
	CSAddrInfo.iSocketType = SOCK_STREAM;
	CSAddrInfo.iProtocol = BTHPROTO_RFCOMM;

	ZeroMemory(&wsaQuerySet, sizeof(WSAQUERYSET));
	wsaQuerySet.dwSize = sizeof(WSAQUERYSET);
	wsaQuerySet.lpServiceClassId = (LPGUID)&ROBOT_NECK_GUID;

	size_t instanceNameSize;
	HRESULT res = StringCchLength(comName, sizeof(comName), &instanceNameSize);
	if (FAILED(res))
	{
		//printf("safe str fail\n");
		return false;
	}

	instanceNameSize += sizeof(INSTANCE_NAME) + 1;
	instanceName = (LPWSTR)HeapAlloc(GetProcessHeap(),
									HEAP_ZERO_MEMORY,
									instanceNameSize);

	StringCbPrintf(instanceName, instanceNameSize, L"%s %s", comName, INSTANCE_NAME);
	wsaQuerySet.lpszServiceInstanceName = instanceName;
	wsaQuerySet.lpszComment = L"Control robot neck with bluetooth(BLE)";
	wsaQuerySet.dwNameSpace = NS_BTH;
	wsaQuerySet.dwNumberOfCsAddrs = 1;      // Must be 1.
	wsaQuerySet.lpcsaBuffer = &CSAddrInfo; // Req'd.

	if (SOCKET_ERROR == WSASetService(&wsaQuerySet, RNRSERVICE_REGISTER, 0))
	{
		//printf("WSA Set Service Fail");
		return false;
	}

	// listen
	if (listen(this->bluetoothSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		//printf("Listen Fail\n");
		return false;
	}

	// start thread
	this->bluetooth_reader = new thread(&MyRobotNeck::ReadBluetooth, this);

	return true;
}

void MyRobotNeck::StopBluetooth()
{
	if (this->bluetoothSocket != INVALID_SOCKET)
	{
		closesocket(this->bluetoothSocket);
		this->bluetoothSocket = INVALID_SOCKET;
	}
	if (this->bluetooth_reader != NULL)
	{
		this->bluetooth_reader->detach();
		delete this->bluetooth_reader;
		this->bluetooth_reader = NULL;
	}
	if (this->instanceName != NULL)
	{
		HeapFree(GetProcessHeap(), 0, this->instanceName);
		this->instanceName = NULL;
	}
}

void MyRobotNeck::GetLimitX(__out double& rxmin, __out double& rxmax)
{
	rxmin = this->limitRXMin;
	rxmax = this->limitRXMax;
}

void MyRobotNeck::GetLimitY(__out double& rymin, __out double& rymax)
{
	rymin = this->limitRYMin;
	rymax = this->limitRYMax;
}

bool MyRobotNeck::Calibration()
{
	if (isSerialPortConnected == false)
		return false;

	const BYTE* calibration = (const BYTE*)"$CAL\r\n";

	for (const BYTE* tempByte = calibration; *tempByte != '\0'; tempByte++)
	{
		if (this->serialPort.WriteByte(*tempByte) == false)
			return false;
	}

	return true;
}

bool MyRobotNeck::Rotation(__in const double& pitch, __in const double& yaw)
{
	if (isSerialPortConnected == false)
		return false;

	double correctionPitch = this->limitRXMin > pitch ? this->limitRXMin : (this->limitRXMax < pitch ? this->limitRXMax : pitch);
	double correctionYaw = this->limitRYMin > yaw ? this->limitRYMin : (this->limitRYMax < yaw ? this->limitRYMax : yaw);
	BYTE rotation[100];

	// limit event occur
	if (this->limitEvent != NULL)
		if (this->limitRXMin > pitch || this->limitRXMax < pitch ||
			this->limitRYMin > yaw || this->limitRYMax < yaw)
			this->limitEvent(pitch, yaw);

	float x = sin(PI*correctionYaw/180) * cos(PI*correctionPitch/180);
	float y = -sin(PI*correctionPitch/180);
	float z = cos(PI*correctionYaw/180) * cos(PI*correctionPitch/180);
	RobotVector3 oculusForward = RobotVector3(x, y, z);
	if (ROT_MIN_ANGLE > oculusForward.Angle(this->forward))
		return false;

	this->forward = oculusForward;
	sprintf((char*)rotation, "$IMU,68,%.2lf,%.2lf,\r\n", correctionYaw, correctionPitch);

	for (const BYTE* tempByte = rotation; *tempByte != '\0'; tempByte++)
	{
		if (this->serialPort.WriteByte(*tempByte) == false)
			return false;
	}

	return true;
}

bool MyRobotNeck::LimitInit()
{
	if (isSerialPortConnected == false)
		return false;

	const BYTE* request = (const BYTE*)"$REQ\r\n";

	for (const BYTE* tempByte = request; *tempByte != '\0'; tempByte++)
	{
		if (this->serialPort.WriteByte(*tempByte) == false)
			return false;
	}

	BYTE ack[100];
	BYTE tempByte;
	int index = 0;
	while (this->serialPort.ReadByte(tempByte))
	{
		if (tempByte == '\n')
		{
			ack[index] = '\0';
			char* temp = reinterpret_cast<char*>(ack);
			char* pch = strtok(temp, ",");
			int splitIndex = 0;
			while (pch != NULL)
			{
				if (splitIndex == 1)
					limitRXMin = atof(pch);
				else if (splitIndex == 2)
					limitRXMax = atof(pch);
				else if (splitIndex == 3)
					limitRYMin = atof(pch);
				else if (splitIndex == 4)
				{
					limitRYMax = atof(pch);
					return true;
				}
				pch = strtok(NULL, ",");
				splitIndex++;
			}
		}
		else
			ack[index++] = tempByte;
	}

	return false;
}

void MyRobotNeck::SetLimitEvent(void(*limitEvent)(__in const double& pitch, __in const double& yaw))
{
	this->limitEvent = limitEvent;
}

#pragma pack(push, 1)
void MyRobotNeck::ReadUDP()
{
	struct sockaddr_in pcaddr;
	int addrlen = sizeof(pcaddr);
	RobotPacket packet;
	int recvlen;

	while (1)
	{
		if (this->udpSocket == INVALID_SOCKET)
		{
			this->udp_reader->detach();
			delete this->udp_reader;
			this->udp_reader = NULL;
			return;
		}
		if (this->isSerialPortConnected == false)
			continue;

		// limit init
		if (this->limitRXMin == 0 || this->limitRXMax == 0 ||
			this->limitRYMin == 0 || this->limitRYMax == 0)
			LimitInit();

		memset(&packet, 0, sizeof(packet));
		if ((recvlen = recvfrom(this->udpSocket, (char*)&packet, sizeof(packet), 0, (SOCKADDR*)&pcaddr, &addrlen)) == SOCKET_ERROR)
		{
			printf("Receive Error: %d\n", WSAGetLastError());
			continue;
		}

		// type check
		if (packet.type == RobotPacketType::CALIB)
		{
			MyRobotNeck::Calibration();
		}
		else if (packet.type == RobotPacketType::ROT)
		{
			MyRobotNeck::Rotation(packet.pitch, packet.yaw);
			// debug
			//printf("Pitch: %lf, Yaw: %lf\n", packet.pitch, packet.yaw);
		}
	}
}
#pragma pack(pop)

#pragma pack(push, 1)
void MyRobotNeck::ReadBluetooth()
{
	while (true)
	{
		// accept one bluetooth client
		this->bClientSocket = accept(this->bluetoothSocket, NULL, NULL);
		if (this->bClientSocket == INVALID_SOCKET)
			return;

		BLE_RobotPacket packet;
		int recvlen;
		bool flush = true;

		// limit init
		if (this->limitRXMin == 0 || this->limitRXMax == 0 ||
			this->limitRYMin == 0 || this->limitRYMax == 0)
			LimitInit();

		// start reading
		while (flush)
		{
			recvlen = recv(this->bClientSocket, (char *)&packet, sizeof(BLE_RobotPacket), MSG_WAITALL);
			switch (recvlen)
			{
			case 0:
				flush = false;
				break;

			case SOCKET_ERROR:
				flush = false;
				break;

			default:
				// make packet and send to robot
				if (packet.type == RobotPacketType::CALIB)
				{
					MyRobotNeck::Calibration();
				}
				else if (packet.type == RobotPacketType::ROT)
				{
					MyRobotNeck::Rotation((double)packet.pitch, (double)packet.yaw);
					// debug
					//printf("Pitch: %lf, Yaw: %lf\n", packet.pitch, packet.yaw);
				}
			}
		}

		// end
		this->bClientSocket = INVALID_SOCKET;
	}
}
#pragma pack(pop)