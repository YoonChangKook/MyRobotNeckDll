// MyAHRSDll.cpp : DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

//#define MY_AHRS_DLL_EXPORTS

#include "stdafx.h"
#include "CSerialPort.h"

CSerialPort::CSerialPort()
{}

CSerialPort::~CSerialPort()
{}

bool CSerialPort::OpenPort(const char* portname)
{
	//char* str = new char[strlen(portname) + 5];
	char str[20];
	strcpy(str, "//./");
	strcpy(str + 4, portname);
	str[strlen(portname) + 4] = '\0';
	//TCHAR* fileStr = new TCHAR[strlen(str)];
	TCHAR fileStr[20];

	mbstowcs(fileStr, str, strlen(str) + 1);

	this->m_hComm = CreateFile(fileStr, 
						GENERIC_READ | GENERIC_WRITE,
						0,
						0,
						OPEN_EXISTING,
						0,
						0);

	//delete[] fileStr;
	//delete[] str;

	if(this->m_hComm == INVALID_HANDLE_VALUE)
		return false;
	else
		return true;
}

bool CSerialPort::Configure(DWORD BaudRate, BYTE ByteSize, DWORD fParity,
								BYTE Parity, BYTE StopBits)
{
	if((this->m_bPortReady = GetCommState(this->m_hComm, &this->m_dcb)) == 0)
	{
		CloseHandle(this->m_hComm);
		return false;
	}

	m_dcb.BaudRate          = BaudRate;  
    m_dcb.ByteSize          = ByteSize;  
    m_dcb.Parity            = Parity ;  
    m_dcb.StopBits          = StopBits;  
    m_dcb.fBinary           = true;  
    m_dcb.fDsrSensitivity   = false;  
    m_dcb.fParity           = fParity;  
    m_dcb.fOutX             = false;  
    m_dcb.fInX              = false;  
    m_dcb.fNull             = false;  
    m_dcb.fAbortOnError     = true;  
    m_dcb.fOutxCtsFlow      = false;  
    m_dcb.fOutxDsrFlow      = false;  
    m_dcb.fDtrControl       = DTR_CONTROL_DISABLE;  
    m_dcb.fDsrSensitivity   = false;  
    m_dcb.fRtsControl       = RTS_CONTROL_DISABLE;  
    m_dcb.fOutxCtsFlow      = false;  
    m_dcb.fOutxCtsFlow      = false;

	this->m_bPortReady = SetCommState(m_hComm, &m_dcb);

	if(this->m_bPortReady == 0)
	{
		CloseHandle(this->m_hComm);
		return false;
	}

	return true;
}

bool CSerialPort::SetCommunicationTimeouts(DWORD ReadIntervalTimeout,
										   DWORD ReadTotalTimeoutMultiplier,
										   DWORD ReadTotalTimeoutConstant,
										   DWORD WriteTotalTimeoutMultiplier,
										   DWORD WriteTotalTimeoutConstant)
{
	if((this->m_bPortReady = GetCommTimeouts(m_hComm, &this->m_CommTimeouts)) == 0)
		return false;

	this->m_CommTimeouts.ReadIntervalTimeout          = ReadIntervalTimeout;  
    this->m_CommTimeouts.ReadTotalTimeoutConstant     = ReadTotalTimeoutConstant;  
    this->m_CommTimeouts.ReadTotalTimeoutMultiplier   = ReadTotalTimeoutMultiplier;  
    this->m_CommTimeouts.WriteTotalTimeoutConstant    = WriteTotalTimeoutConstant;  
    this->m_CommTimeouts.WriteTotalTimeoutMultiplier  = WriteTotalTimeoutMultiplier;

	this->m_bPortReady = SetCommTimeouts(this->m_hComm, &this->m_CommTimeouts);

	if(this->m_bPortReady == 0)
	{
		CloseHandle(this->m_hComm);
		return false;
	}

	return true;
}

bool CSerialPort::WriteByte(BYTE byte)
{
	this->m_iBytesWritten = 0;
	if(WriteFile(this->m_hComm, &byte, 1, &this->m_iBytesWritten, NULL) == 0)
		return false;
	else
		return true;
}

bool CSerialPort::ReadByte(BYTE& resp)
{
	BYTE rx;
	resp = 0;

	DWORD dwBytesTranferred = 0;

	if(ReadFile(this->m_hComm, &rx, 1, &dwBytesTranferred, 0))
	{
		if(dwBytesTranferred == 1)
		{
			resp = rx;
			return true;
		}
	}

	return false;
}

bool CSerialPort::ReadByte(BYTE* &resp, UINT size)
{
	DWORD dwBytesTransferred = 0;

	if(ReadFile(this->m_hComm, resp, size, &dwBytesTransferred, 0))
	{
		if(dwBytesTransferred == size)
			return true;
	}

	return false;
}

void CSerialPort::ClosePort()
{
	CloseHandle(this->m_hComm);
	return;
}