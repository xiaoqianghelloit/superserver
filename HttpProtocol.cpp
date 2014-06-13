#include "stdafx.h"
#include "HttpProtocol.h"

// ��������ʱ�������ת��
char *week[] = {		
	"Sun,",  
	"Mon,",
	"Tue,",
	"Wed,",
	"Thu,",
	"Fri,",
	"Sat,",
};
 
// ��������ʱ����·�ת��
char *month[] = 
{	
	"Jan",  
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec",
};


UINT CHttpProtocol::ClientNum = 0;
CCriticalSection CHttpProtocol::m_critSect;		// �ų�����ʼ��
HANDLE	CHttpProtocol::None = NULL;


CHttpProtocol::CHttpProtocol(void)
{
	m_pListenThread = NULL;	
	//m_hwndDlg = NULL;
}

CHttpProtocol::~CHttpProtocol(void)
{
}

bool CHttpProtocol::StartHttpSrv()
{
	WORD wVersionRequested = WINSOCK_VERSION;
	WSADATA wsaData;
	int nRet;
	// ����Winsock
	nRet = WSAStartup(wVersionRequested, &wsaData);		// ���سɹ�����0
	if (nRet)
	{   
		// ������
		AfxMessageBox("Initialize WinSock Failed");
		return false;
	}
	// ���汾
	if (wsaData.wVersion != wVersionRequested)
	{    
		// ������   
		AfxMessageBox("Wrong WinSock Version");
		return false;
	}
	
	m_hExit = CreateEvent(NULL, TRUE, FALSE, NULL);	
	if (m_hExit == NULL)
	{
		return false;
	}

	//�����׽���
	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_listenSocket == INVALID_SOCKET)
	{
		// �쳣����
		printf("error: Could not create listen socket\n");
		return false;
	}

	SOCKADDR_IN sockAddr;
	LPSERVENT	lpServEnt;
	if (m_nPort != 0)
	{
		// �������ֽ�˳��תΪ�����ֽ�˳�򸳸�sin_port
		sockAddr.sin_port = htons(m_nPort);
	}
	else
	{	
		// ��ȡ��֪http����Ķ˿ڣ��÷�����tcpЭ����ע��
		lpServEnt = getservbyname("http", "tcp");
		if (lpServEnt != NULL)
		{
			sockAddr.sin_port = lpServEnt->s_port;
		}
		else
		{
			sockAddr.sin_port = htons(HTTPPORT);	// Ĭ�϶˿�HTTPPORT��80
		}
	}

	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = INADDR_ANY;  // ָ���˿ں������Ĭ��IP�ӿ� 

	// ��ʼ��content-type���ļ���׺��Ӧ��ϵ��map
    CreateTypeMap();


	// �׽��ְ�
	nRet = bind(m_listenSocket, (LPSOCKADDR)&sockAddr, sizeof(struct sockaddr));
	if (nRet == SOCKET_ERROR)
	{  
		// �󶨷�������
		printf("error bind() error\n");
		closesocket(m_listenSocket);	// �Ͽ�����
		return false;
	}

    // �׽��ּ�����Ϊ�ͻ����Ӵ����ȴ�����,������󳤶�SOMAXCONN��windows socketsͷ�ļ��ж���
	nRet = listen(m_listenSocket, SOMAXCONN);
	if (nRet == SOCKET_ERROR)
	{   
		// �쳣����
		printf("error listen() error\n");
		closesocket(m_listenSocket);	// �Ͽ�����
		return false;
	}
	// ����listening���̣����ܿͻ�������Ҫ��
	m_pListenThread = AfxBeginThread(ListenThread, this);
	
	if (!m_pListenThread)
	{
		// �̴߳���ʧ��
		printf("error Could not create listening thread\n");
		closesocket(m_listenSocket);	// �Ͽ�����
		return false;
	}
	char* strIP=NULL;
	char hostname[255];
	PHOSTENT hostinfo;
	// ��ȡ�������
	if(gethostname(hostname, sizeof(hostname))==0)	// �õ�������
	{
		// ���������õ�������������Ϣ
		hostinfo = gethostbyname(hostname);
		if(hostinfo != NULL)
		{
			strIP = inet_ntoa(*(struct in_addr*)*(hostinfo->h_addr_list));
		}
	}
	
	// ��ʾweb��������������
	printf("****** My WebServer is Starting now! *******\n");

	// ��ʾweb����������Ϣ��������������IP�Լ��˿ں�
	//CString *pStr1 = new CString;
	//pStr1->Format("%s", hostname); 
	//*pStr1 = *pStr1 + "[" + strIP + "]" + "   Port ";
	//strTemp.Format("%d", htons(sockAddr.sin_port));
	//*pStr1 = *pStr1 + strTemp;
	printf("%s [ %s ] %d\n",hostname,strIP,htons(sockAddr.sin_port));
	WaitForSingleObject(m_pListenThread->m_hThread,INFINITE);
	return true;

}


void CHttpProtocol::CreateTypeMap()
{
	// ��ʼ��map
    m_typeMap[".doc"]	= "application/msword";
	m_typeMap[".bin"]	= "application/octet-stream";
	m_typeMap[".dll"]	= "application/octet-stream";
	m_typeMap[".exe"]	= "application/octet-stream";
	m_typeMap[".pdf"]	= "application/pdf";
	m_typeMap[".ai"]	= "application/postscript";
	m_typeMap[".eps"]	= "application/postscript";
	m_typeMap[".ps"]	= "application/postscript";
	m_typeMap[".rtf"]	= "application/rtf";
	m_typeMap[".fdf"]	= "application/vnd.fdf";
	m_typeMap[".arj"]	= "application/x-arj";
	m_typeMap[".gz"]	= "application/x-gzip";
	m_typeMap[".class"]	= "application/x-java-class";
	m_typeMap[".js"]	= "application/x-javascript";
	m_typeMap[".lzh"]	= "application/x-lzh";
	m_typeMap[".lnk"]	= "application/x-ms-shortcut";
	m_typeMap[".tar"]	= "application/x-tar";
	m_typeMap[".hlp"]	= "application/x-winhelp";
	m_typeMap[".cert"]	= "application/x-x509-ca-cert";
	m_typeMap[".zip"]	= "application/zip";
	m_typeMap[".cab"]	= "application/x-compressed";
	m_typeMap[".arj"]	= "application/x-compressed";
	m_typeMap[".aif"]	= "audio/aiff";
	m_typeMap[".aifc"]	= "audio/aiff";
	m_typeMap[".aiff"]	= "audio/aiff";
	m_typeMap[".au"]	= "audio/basic";
	m_typeMap[".snd"]	= "audio/basic";
	m_typeMap[".mid"]	= "audio/midi";
	m_typeMap[".rmi"]	= "audio/midi";
	m_typeMap[".mp3"]	= "audio/mpeg";
	m_typeMap[".vox"]	= "audio/voxware";
	m_typeMap[".wav"]	= "audio/wav";
	m_typeMap[".ra"]	= "audio/x-pn-realaudio";
	m_typeMap[".ram"]	= "audio/x-pn-realaudio";
	m_typeMap[".bmp"]	= "image/bmp";
	m_typeMap[".gif"]	= "image/gif";
	m_typeMap[".jpeg"]	= "image/jpeg";
	m_typeMap[".jpg"]	= "image/jpeg";
	m_typeMap[".tif"]	= "image/tiff";
	m_typeMap[".tiff"]	= "image/tiff";
	m_typeMap[".xbm"]	= "image/xbm";
	m_typeMap[".wrl"]	= "model/vrml";
	m_typeMap[".htm"]	= "text/html";
	m_typeMap[".html"]	= "text/html";
	m_typeMap[".c"]		= "text/plain";
	m_typeMap[".cpp"]	= "text/plain";
	m_typeMap[".def"]	= "text/plain";
	m_typeMap[".h"]		= "text/plain";
	m_typeMap[".txt"]	= "text/plain";
	m_typeMap[".rtx"]	= "text/richtext";
	m_typeMap[".rtf"]	= "text/richtext";
	m_typeMap[".java"]	= "text/x-java-source";
	m_typeMap[".css"]	= "text/css";
	m_typeMap[".mpeg"]	= "video/mpeg";
	m_typeMap[".mpg"]	= "video/mpeg";
	m_typeMap[".mpe"]	= "video/mpeg";
	m_typeMap[".avi"]	= "video/msvideo";
	m_typeMap[".mov"]	= "video/quicktime";
	m_typeMap[".qt"]	= "video/quicktime";
	m_typeMap[".shtml"]	= "wwwserver/html-ssi";
	m_typeMap[".asa"]	= "wwwserver/isapi";
	m_typeMap[".asp"]	= "wwwserver/isapi";
	m_typeMap[".cfm"]	= "wwwserver/isapi";
	m_typeMap[".dbm"]	= "wwwserver/isapi";
	m_typeMap[".isa"]	= "wwwserver/isapi";
	m_typeMap[".plx"]	= "wwwserver/isapi";
	m_typeMap[".url"]	= "wwwserver/isapi";
	m_typeMap[".cgi"]	= "wwwserver/isapi";
	m_typeMap[".php"]	= "wwwserver/isapi";
	m_typeMap[".wcgi"]	= "wwwserver/isapi";

}


UINT CHttpProtocol::ListenThread(LPVOID param)
{
	CHttpProtocol *pHttpProtocol = (CHttpProtocol *)param;

	SOCKET		socketClient;
	CWinThread*	pClientThread;
	SOCKADDR_IN	SockAddr;
	PREQUEST	pReq;
	int			nLen;
	DWORD		dwRet;

	// ��ʼ��ClientNum������"no client"�¼�����
	HANDLE		hNoClients;
	hNoClients = pHttpProtocol->InitClientCount();

	while(1)	// ѭ���ȴ�,���пͻ���������,����ܿͻ�������Ҫ��
	{	
		nLen = sizeof(SOCKADDR_IN);		
		// �׽��ֵȴ�����,���ض�Ӧ�ѽ��ܵĿͻ������ӵ��׽���
		socketClient = accept(pHttpProtocol->m_listenSocket, (LPSOCKADDR)&SockAddr, &nLen);
		if (socketClient == INVALID_SOCKET)
		{   
			break;
		}		
		// ���ͻ��������ַת��Ϊ�õ�ָ��IP��ַ
		CString *pstr = new CString;
		pstr->Format("%s Connecting on socket:%d", inet_ntoa(SockAddr.sin_addr), socketClient);
		//SendMessage(pHttpProtocol->m_hwndDlg, LOG_MSG, (UINT)pstr, NULL);

        pReq = new REQUEST;
		if (pReq == NULL)
		{   
			// �������
			CString *pStr = new CString;
			*pStr = "No memory for request";
			//SendMessage(pHttpProtocol->m_hwndDlg, LOG_MSG, (UINT)pStr, NULL);
			continue;
		}
		pReq->hExit  = pHttpProtocol->m_hExit;
		pReq->Socket = socketClient;
		pReq->hFile = INVALID_HANDLE_VALUE;
		pReq->dwRecv = 0;
		pReq->dwSend = 0;
		pReq->pHttpProtocol = pHttpProtocol;

	    // ����client���̣�����request
		pClientThread = AfxBeginThread(ClientThread, pReq);
		if (!pClientThread)
		{  
			// �̴߳���ʧ��,������
			CString *pStr = new CString;
			*pStr = "Couldn't start client thread";
			//SendMessage(pHttpProtocol->m_hwndDlg, LOG_MSG, (UINT)pStr, NULL);

			delete pReq;
		}
	} //while
	// �ȴ��߳̽���
	WaitForSingleObject((HANDLE)pHttpProtocol->m_hExit, INFINITE);
    // �ȴ�����client���̽���
	dwRet = WaitForSingleObject(hNoClients, 5000);
	if (dwRet == WAIT_TIMEOUT) 
	{  
		// ��ʱ���أ�����ͬ������δ�˳�
		CString *pStr = new CString;
		*pStr = "One or more client threads did not exit";
		//SendMessage(pHttpProtocol->m_hwndDlg, LOG_MSG, (UINT)pStr, NULL);
	}
	pHttpProtocol->DeleteClientCount();

	return 0;
}

UINT CHttpProtocol::ClientThread(LPVOID param)
{
	int nRet;
	BYTE buf[1024];
	PREQUEST pReq = (PREQUEST)param;
	CHttpProtocol *pHttpProtocol = (CHttpProtocol *)pReq->pHttpProtocol;

	pHttpProtocol->CountUp();			// ����
	
	// ����request data
	 if (!pHttpProtocol->RecvRequest(pReq, buf, sizeof(buf)))
	{  
		pHttpProtocol->Disconnect(pReq);
		delete pReq;
		pHttpProtocol->CountDown();      
		return 0;  
	}
	
	// ����request��Ϣ
	nRet = pHttpProtocol->Analyze(pReq, buf);
	if (nRet)
	{	
		// �������
		CString *pStr = new CString;
		*pStr = "Error occurs when analyzing client request";
		//SendMessage(pHttpProtocol->m_hwndDlg, LOG_MSG, (UINT)pStr, NULL);

		pHttpProtocol->Disconnect(pReq);
		delete pReq;
		pHttpProtocol->CountDown();     
		return 0;
	}

	// ���ɲ�����ͷ��
	pHttpProtocol->SendHeader(pReq);

	// ��client��������
	if(pReq->nMethod == METHOD_GET)
	{
		pHttpProtocol->SendFile(pReq);
	}

	pHttpProtocol->Disconnect(pReq);
	delete pReq;
	pHttpProtocol->CountDown();	// client������1

	return 0;
}

bool CHttpProtocol::RecvRequest(PREQUEST pReq, LPBYTE pBuf, DWORD dwBufSize)
{
	WSABUF			wsabuf;		// ����/���ջ������ṹ
	WSAOVERLAPPED	over;		// ָ������ص�����ʱָ����WSAOVERLAPPED�ṹ
	DWORD			dwRecv;
	DWORD			dwFlags;
	DWORD			dwRet;
	HANDLE			hEvents[2];
	bool			fPending;
	int				nRet;
	memset(pBuf, 0, dwBufSize);	// ��ʼ��������
    wsabuf.buf  = (char *)pBuf;
	wsabuf.len  = dwBufSize;	// �������ĳ���
	over.hEvent = WSACreateEvent();	// ����һ���µ��¼�����
	dwFlags = 0;
	fPending = FALSE;  
	// ��������
	nRet = WSARecv(pReq->Socket, &wsabuf, 1, &dwRecv, &dwFlags, &over, NULL);
    if (nRet != 0)  
	{
    	// �������WSA_IO_PENDING��ʾ�ص������ɹ�����
		if (WSAGetLastError() != WSA_IO_PENDING)
		{   
			// �ص�����δ�ܳɹ�
			CloseHandle(over.hEvent);
			return false;
		}
		else	
		{
			fPending = true;
		}
	}
	if (fPending)
	{	
		hEvents[0]  = over.hEvent;
		hEvents[1]  = pReq->hExit;
		dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		if (dwRet != 0)
		{
			CloseHandle(over.hEvent);                     
			return false;
		}
        // �ص�����δ���
		if (!WSAGetOverlappedResult(pReq->Socket, &over, &dwRecv, FALSE, &dwFlags))
	{
			CloseHandle(over.hEvent);                    
		    return false;
		}
	}
	pReq->dwRecv += dwRecv;	// ͳ�ƽ�������
	CloseHandle(over.hEvent);                           
	return true;
}
	
// ����request��Ϣ
int  CHttpProtocol::Analyze(PREQUEST pReq, LPBYTE pBuf)
{
	// �������յ�����Ϣ
	char szSeps[] = " \n";
	char *cpToken;
	// ��ֹ�Ƿ�����
	if (strstr((const char *)pBuf, "..") != NULL)
	{
		strcpy(pReq->StatuCodeReason, HTTP_STATUS_BADREQUEST);
		return 1;
	}

	// �ж�ruquest��mothed	
	cpToken = strtok((char *)pBuf, szSeps);	// �������ַ����ֽ�Ϊһ���Ǵ���	
	if (!_stricmp(cpToken, "GET"))			// GET����
	{
		pReq->nMethod = METHOD_GET;
	}
	else if (!_stricmp(cpToken, "HEAD"))	// HEAD����
	{
		pReq->nMethod = METHOD_HEAD;  
	}
	else  
	{
        strcpy(pReq->StatuCodeReason, HTTP_STATUS_NOTIMPLEMENTED);
		return 1;
	}
    
	// ��ȡRequest-URI
	cpToken = strtok(NULL, szSeps);
	if (cpToken == NULL)   
	{
		strcpy(pReq->StatuCodeReason, HTTP_STATUS_BADREQUEST);
		return 1;
	}

	strcpy(pReq->szFileName, m_strRootDir);
	if (strlen(cpToken) > 1)
	{
		strcat(pReq->szFileName, cpToken);	// �Ѹ��ļ�����ӵ���β���γ�·��
	}
	else
	{
		strcat(pReq->szFileName, "/index.html");
	}
	return 0;
}

// ����ͷ��
void CHttpProtocol::SendHeader(PREQUEST pReq)
{
	
    int n = FileExist(pReq);
	if(!n)		// �ļ������ڣ��򷵻�
	{
		return;
	}

	char Header[2048] = " ";
	char curTime[50] = " ";
	GetCurentTime((char*)curTime);
	// ȡ���ļ�����
    DWORD length;
	length = GetFileSize(pReq->hFile, NULL);	
	// ȡ���ļ���last-modifiedʱ��
	char last_modified[60] = " ";
	GetLastModified(pReq->hFile, (char*)last_modified);	
	// ȡ���ļ�������
	char ContenType[50] = " ";
 	GetContenType(pReq, (char*)ContenType);

	sprintf((char*)Header, "HTTP/1.0 %s\r\nDate: %s\r\nServer: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nLast-Modified: %s\r\n\r\n",
			                    HTTP_STATUS_OK, 
								curTime,				// Date
								"My Http Server",       // Server
								ContenType,				// Content-Type
								length,					// Content-length
								last_modified);			// Last-Modified

    // ����ͷ��
	send(pReq->Socket, Header, strlen(Header), 0);	
}


int CHttpProtocol::FileExist(PREQUEST pReq)
{
	pReq->hFile = CreateFile(pReq->szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	// ����ļ������ڣ��򷵻س�����Ϣ
	if (pReq->hFile == INVALID_HANDLE_VALUE)
	{
		strcpy(pReq->StatuCodeReason, HTTP_STATUS_NOTFOUND);
		return 0;
	}
	else 
	{
		return 1;
	}
}

// �����ļ�
void CHttpProtocol::SendFile(PREQUEST pReq)
{
	int n = FileExist(pReq);
	if(!n)			// �ļ������ڣ��򷵻�
	{		
		return;
	}

	CString *pStr = new CString;
	*pStr = *pStr + &pReq->szFileName[strlen(m_strRootDir)];
	//SendMessage(m_hwndDlg, LOG_MSG, UINT(pStr), NULL);	
   

	static BYTE buf[2048];
    DWORD  dwRead;
    BOOL   fRet;
	int flag = 1;
    // ��д����ֱ�����
    while(1)
	{	
		// ��file�ж��뵽buffer��        
		fRet = ReadFile(pReq->hFile, buf, sizeof(buf), &dwRead, NULL);
		if (!fRet)
		{	
			
	    	static char szMsg[512];
		    wsprintf(szMsg, "%s", HTTP_STATUS_SERVERERROR);
        	// ��ͻ��˷��ͳ�����Ϣ
        	send(pReq->Socket, szMsg, strlen(szMsg), 0);	
	    	break;
		}
		// ���
		if (dwRead == 0)
		{	
			break;
		}
		// ��buffer���ݴ��͸�client
		if (!SendBuffer(pReq, buf, dwRead))	
		{
			break;
		}
		pReq->dwSend += dwRead;
	}
    // �ر��ļ�
	if (CloseHandle(pReq->hFile))
	{
		pReq->hFile = INVALID_HANDLE_VALUE;
	}
	else
	{
		CString *pStr = new CString;
		*pStr = "Error occurs when closing file";
		//SendMessage(m_hwndDlg, LOG_MSG, (UINT)pStr, NULL);
	}
}

bool CHttpProtocol::SendBuffer(PREQUEST pReq, LPBYTE pBuf, DWORD dwBufSize)
{   
	// ���ͻ����е�����
	WSABUF			wsabuf;
	WSAOVERLAPPED	over;
	DWORD			dwRecv;
	DWORD			dwFlags;
	DWORD			dwRet;
	HANDLE			hEvents[2];
	BOOL			fPending;
	int				nRet;
	wsabuf.buf  = (char *)pBuf;
	wsabuf.len  = dwBufSize;
	over.hEvent = WSACreateEvent();
	fPending = false;
	
	// �������� 
	nRet = WSASend(pReq->Socket, &wsabuf, 1, &dwRecv, 0, &over, NULL);
	if (nRet != 0)
	{
		// �������
		if (WSAGetLastError() == WSA_IO_PENDING)
		{
			fPending = true;
		}
		else
		{	
			CString *pStr = new CString;
			pStr->Format("WSASend() error: %d", WSAGetLastError() );
			//SendMessage(m_hwndDlg, LOG_MSG, (UINT)pStr, NULL);

			CloseHandle(over.hEvent);
			return false;
		}
	}
	if (fPending)	// i/oδ���
	{	
		hEvents[0]  = over.hEvent;
		hEvents[1]  = pReq->hExit;
		dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		if (dwRet != 0)
		{
			CloseHandle(over.hEvent);
			return false;
		}
		// �ص�����δ���
		if (!WSAGetOverlappedResult(pReq->Socket, &over, &dwRecv, FALSE, &dwFlags))
		{
			// ������
			CString *pStr = new CString;
			pStr->Format("WSAGetOverlappedResult() error: %d", WSAGetLastError() );
			//SendMessage(m_hwndDlg, LOG_MSG, (UINT)pStr, NULL);
			CloseHandle(over.hEvent);
			return false;
		}
	}
	CloseHandle(over.hEvent);
	return true;
}

void CHttpProtocol::Disconnect(PREQUEST pReq)
{
	// �ر��׽��֣��ͷ���ռ�е���Դ
	int	nRet;
	CString *pStr = new CString;
	pStr->Format("Closing socket: %d", pReq->Socket);
	//SendMessage(m_hwndDlg, LOG_MSG, (UINT)pStr, NULL);
	
	nRet = closesocket(pReq->Socket);
	if (nRet == SOCKET_ERROR)
	{
		// �������
		CString *pStr1 = new CString;
		pStr1->Format("closesocket() error: %d", WSAGetLastError() );
		//SendMessage(m_hwndDlg, LOG_MSG, (UINT)pStr1, NULL);
	}

	HTTPSTATS	stats;
	stats.dwRecv = pReq->dwRecv;
	stats.dwSend = pReq->dwSend;
	//SendMessage(m_hwndDlg, DATA_MSG, (UINT)&stats, NULL);
}

HANDLE CHttpProtocol::InitClientCount()
{
	ClientNum = 0;
	// ����"no client"�¼�����
	None = CreateEvent(NULL, TRUE, TRUE, NULL);	
	return None;
}

void CHttpProtocol::CountUp()
{
	// �����ų���
	m_critSect.Lock();
	ClientNum++;
	// �뿪�ų���
	m_critSect.Unlock();
	// ����Ϊ���ź��¼�����
	ResetEvent(None);
}

void CHttpProtocol::CountDown()
{	
	// �����ų���
	m_critSect.Lock();
	if(ClientNum > 0)
	{
		ClientNum--;
	} 
	// �뿪�ų���
	m_critSect.Unlock();
	if(ClientNum < 1)
	{
		// ����Ϊ���ź��¼�����
		SetEvent(None);
	}
}

void CHttpProtocol::DeleteClientCount()
{
	CloseHandle(None);
}

// �����ʱ��
void CHttpProtocol::GetCurentTime(LPSTR lpszString)
{  
	// �����ʱ��
	SYSTEMTIME st;
	GetLocalTime(&st);
	// ʱ���ʽ��
    wsprintf(lpszString, "%s %02d %s %d %02d:%02d:%02d GMT",week[st.wDayOfWeek], st.wDay,month[st.wMonth-1],
     st.wYear, st.wHour, st.wMinute, st.wSecond);
}

bool CHttpProtocol::GetLastModified(HANDLE hFile, LPSTR lpszString)
{
	// ����ļ���last-modified ʱ��
	FILETIME ftCreate, ftAccess, ftWrite;
    SYSTEMTIME stCreate;
	FILETIME ftime;	
	// ����ļ���last-modified��UTCʱ��
    if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))   
		return false;
	FileTimeToLocalFileTime(&ftWrite,&ftime);
	// UTCʱ��ת���ɱ���ʱ��
    FileTimeToSystemTime(&ftime, &stCreate);	
	// ʱ���ʽ��
	wsprintf(lpszString, "%s %02d %s %d %02d:%02d:%02d GMT", week[stCreate.wDayOfWeek],
		stCreate.wDay, month[stCreate.wMonth-1], stCreate.wYear, stCreate.wHour,
		stCreate.wMinute, stCreate.wSecond);
}

bool CHttpProtocol::GetContenType(PREQUEST pReq, LPSTR type)
{   
	// ȡ���ļ�������
    CString cpToken;
    cpToken = strstr(pReq->szFileName, ".");
    strcpy(pReq->postfix, cpToken);
	// �����������ļ����Ͷ�Ӧ��content-type
	map<CString, char *>::iterator it = m_typeMap.find(pReq->postfix);
	if(it != m_typeMap.end()) 	
	{
		wsprintf(type,"%s",(*it).second);
	}
	return TRUE;
}

void CHttpProtocol::StopHttpSrv()
{

	int nRet;
	SetEvent(m_hExit);
	nRet = closesocket(m_listenSocket);
	nRet = WaitForSingleObject((HANDLE)m_pListenThread, 10000);
	if (nRet == WAIT_TIMEOUT)
	{
		CString *pStr = new CString;
		*pStr = "TIMEOUT waiting for ListenThread";
		//SendMessage(m_hwndDlg, LOG_MSG, (UINT)pStr, NULL);
	}
	CloseHandle(m_hExit);

	CString *pStr1 = new CString;
	*pStr1 = "Server Stopped";
	//SendMessage(m_hwndDlg, LOG_MSG, (UINT)pStr1, NULL);
}