#ifndef _HTTPPROTOCOL_H
#define _HTTPPROTOCOL_H


#pragma once

#include <winsock2.h>
#include <afxmt.h>

#define HTTPPORT 80
#define HTTPROOTDIR "C://temp/"
#define METHOD_GET 0
#define METHOD_HEAD 1


#define HTTP_STATUS_OK				"200 OK"
#define HTTP_STATUS_CREATED			"201 Created"
#define HTTP_STATUS_ACCEPTED		"202 Accepted"
#define HTTP_STATUS_NOCONTENT		"204 No Content"
#define HTTP_STATUS_MOVEDPERM		"301 Moved Permanently"
#define HTTP_STATUS_MOVEDTEMP		"302 Moved Temporarily"
#define HTTP_STATUS_NOTMODIFIED		"304 Not Modified"
#define HTTP_STATUS_BADREQUEST		"400 Bad Request"
#define HTTP_STATUS_UNAUTHORIZED	"401 Unauthorized"
#define HTTP_STATUS_FORBIDDEN		"403 Forbidden"
#define HTTP_STATUS_NOTFOUND		"404 File can not fonund!"
#define HTTP_STATUS_SERVERERROR		"500 Internal Server Error"
#define HTTP_STATUS_NOTIMPLEMENTED	"501 Not Implemented"
#define HTTP_STATUS_BADGATEWAY		"502 Bad Gateway"
#define HTTP_STATUS_UNAVAILABLE		"503 Service Unavailable"


// ���ӵ�Client����Ϣ
typedef struct REQUEST
{
	HANDLE		hExit;
	SOCKET		Socket;                // �����socket
	int			nMethod;               // �����ʹ�÷�����GET��HEAD
	DWORD		dwRecv;                // �յ����ֽ���
	DWORD		dwSend;                // ���͵��ֽ���
	HANDLE		hFile;                 // �������ӵ��ļ�
	char		szFileName[_MAX_PATH]; // �ļ������·��
	char		postfix[10];           // �洢��չ��
	char        StatuCodeReason[100];  // ͷ����status cod�Լ�reason-phrase
	bool        permitted;             // �û�Ȩ���ж�
	char *      authority;             // �û��ṩ����֤��Ϣ
	char        key[1024];             // ��ȷ��֤��Ϣ

	void* pHttpProtocol;			   // ָ����CHttpProtocol��ָ��
}REQUEST, *PREQUEST;

typedef struct HTTPSTATS
{
	DWORD	dwRecv;               // �յ��ֽ���
	DWORD	dwSend;               // �����ֽ���
}HTTPSTATS, *PHTTPSTATS;


#include <map>
#include <string>
using namespace std;


class CHttpProtocol
{
public:

	//HWND m_hwndDlg;

	SOCKET m_listenSocket;

	map<CString, char *> m_typeMap;		// ����content-type���ļ���׺�Ķ�Ӧ��ϵmap

	CWinThread* m_pListenThread;

	HANDLE m_hExit;

	static HANDLE	None;				// ��־�Ƿ���Client���ӵ�Server
	static UINT ClientNum;				// ���ӵ�Client����
	static CCriticalSection m_critSect; // �������

	CString m_strRootDir;				// web�ĸ�Ŀ¼
	UINT	m_nPort;					// http server�Ķ˿ں�


public:
	CHttpProtocol(void);

	void DeleteClientCount();
	void CountDown();
	void CountUp();
	HANDLE InitClientCount();
	
	void StopHttpSrv();
	bool StartHttpSrv();

	static UINT ListenThread(LPVOID param);
	static UINT ClientThread(LPVOID param);

	bool RecvRequest(PREQUEST pReq, LPBYTE pBuf, DWORD dwBufSize);
	int Analyze(PREQUEST pReq, LPBYTE pBuf);
	void Disconnect(PREQUEST pReq);
	void CreateTypeMap();
	void SendHeader(PREQUEST pReq);
	int FileExist(PREQUEST pReq);
	
	void GetCurentTime(LPSTR lpszString);
	bool GetLastModified(HANDLE hFile, LPSTR lpszString);
	bool GetContenType(PREQUEST pReq, LPSTR type);
	void SendFile(PREQUEST pReq);
	bool SendBuffer(PREQUEST pReq, LPBYTE pBuf, DWORD dwBufSize);


public:
	~CHttpProtocol(void);
};


#endif _HTTPPROTOCOL_H