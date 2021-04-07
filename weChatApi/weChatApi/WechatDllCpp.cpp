﻿#include "pch.h"
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atlconv.h>
#include "malloc.h"
#define HOOK_LEN 5


#define CURL_STATICLIB  //必须在包含curl.h前定义
//以下四项是必须的
#include <curl/curl.h>
#include <curl/easy.h>
#pragma comment ( lib, "ws2_32.lib" )
#pragma comment ( lib, "winmm.lib" )
#pragma comment ( lib, "wldap32.lib" )
#define POSTURL "http://127.0.0.1:5000/recvmessage"
// 需要下面的库才能通过build，具体为什么需要，以后再研究吧
#define BURSIZE 2048
#include <tchar.h>

HANDLE hWHND = 0;
DWORD WinAdd = 0;
HWND hDlg = 0;
DWORD hookAdd = 0;
DWORD retAdd = 0;
BYTE backCode[HOOK_LEN] = { 0 };

DWORD cEax = 0;
DWORD cEcx = 0;
DWORD cEdx = 0;
DWORD cEbx = 0;
DWORD cEsp = 0;
DWORD cEbp = 0;
DWORD cEsi = 0;
DWORD cEdi = 0;

//消息结构体
struct wechatText
{
	wchar_t * pStr;
	int strLen;
	int iStrLen;
	int fill = 0;
	int fill2 = 0;
};

//获取模块基址
DWORD getModuleAddress()
{
	return (DWORD)LoadLibrary(L"WeChatWin.dll");
}

/*
======================发文本消息============================
*/
//发送文本消息函数
VOID SendTextMessage(wchar_t * wxid, wchar_t * message)
{
	wechatText pWxid = { 0 };
	pWxid.pStr = wxid;
	pWxid.strLen = wcslen(wxid);
	pWxid.iStrLen = wcslen(wxid) * 2;

	wechatText pMessage = { 0 };
	pMessage.pStr = message;
	pMessage.strLen = wcslen(message);
	pMessage.iStrLen = wcslen(message) * 2;
	char* asmWxid = (char*)&pWxid.pStr;
	char* asmMsg = (char*)&pMessage.pStr;

	//缓冲区
	char buff[0x880] = { 0 };

	//call地址
	//消息发送 3.0.0.57 0x38D8A0
	//消息发送 3.2.1.127 0x3B56A0
	DWORD callAdd = getModuleAddress() + 0x3B56A0;
	__asm {
		mov edx, asmWxid
		push 0x1
		mov edi, 0x0
		push edi
		mov ebx, asmMsg
		push ebx
		lea ecx, buff
		call callAdd
		add esp, 0xC
	}
	
}




/*
======================发文件消息============================
*/
//微信ID的结构体
struct Wxid
{
	wchar_t* str;
	int strLen = 0;
	int maxLen = 0;
	char file[0x8] = { 0 };
};

struct filePath
{
	wchar_t* str;
	int strLen = 0;
	int maxLen = 0;
	char file[0x18] = { 0 };
};
//发送文件call调用
VOID SendFileMessage(wchar_t * wxid, wchar_t * filepath1)
{
	//wchar_t wxid[0x100] = L"filehelper";
	wchar_t filepath[0x100] = L"";
	swprintf(filepath,L"%s", filepath1);

	//文件消息发送 3.2.1.127
	//516B0000 wechatwin
	//push 17DAB00
	//call 58CBC0
	//call 58 CC00
	//call 58 CC00
	//call 6 8A40
	//call 2C 0BE0
	//构造需要的地址
	DWORD dwBase = getModuleAddress();
	DWORD dwParams = dwBase + 0x17DAB00;
	DWORD dwCall0 = dwBase + 0x58CC00;
	DWORD dwCall1 = dwBase + 0x58CBC0;
	DWORD dwCall2 = dwBase + 0x58CC00;
	DWORD dwCall3 = dwBase + 0x68A40;	//组合数据
	DWORD dwCall4 = dwBase + 0x2C0BE0;	//发送消息


	char buff[0x45C] = { 0 };

	//构造需要的数据
	Wxid wxidStruct = { 0 };
	wxidStruct.str = wxid;
	wxidStruct.strLen = wcslen(wxid);
	wxidStruct.maxLen = wcslen(wxid) * 2;

	filePath filePathStruct = { 0 };
	filePathStruct.str = filepath;
	filePathStruct.strLen = wcslen(filepath);
	filePathStruct.maxLen = wcslen(filepath) * 2;

	//取出需要的数据的地址
	char* pFilePath = (char*)&filePathStruct.str;
	char* pWxid = (char*)&wxidStruct.str;

	__asm {
		pushad
		mov eax, 0x1;
		cmovne ecx, eax;
		sub esp, 0x14;
		mov byte ptr ss : [ebp - 0x6C], cl;
		lea eax, dword ptr ds : [edi - 0x20];
		mov ecx, esp;
		mov dword ptr ss : [ebp - 0x44], esp;
		push eax;
		call dwCall0;
		push dword ptr ss : [ebp - 0x6C];
		sub esp, 0x14;
		mov ecx, esp;
		push - 0x1;
		push dwParams;
		call dwCall1;
		sub esp, 0x14;
		mov ecx, esp;
		push pFilePath;
		call dwCall2;
		sub esp, 0x14;
		lea eax, dword ptr ss : [ebp - 0x88];
		mov ecx, esp;
		push pWxid;
		call dwCall2;
		lea eax, buff;
		push eax;
		call dwCall3;
		mov ecx, eax;
		call dwCall4;
		popad
	}

};


/*
======================消息接收内容============================
*/


//将wchat_t类型数组转成CHAR类型数组
char* UnicodeToChar(const wchar_t* unicode)
{
	int len;
	len = WideCharToMultiByte(CP_UTF8, 0, unicode, -1, NULL, 0, NULL, NULL);
	char *szCHAR = (char*)malloc(len + 1);
	memset(szCHAR, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, unicode, -1, szCHAR, len, NULL, NULL);
	return szCHAR;
}
//显示数据
VOID printLog(DWORD msgAdd)
{
	//信息块的位置
	DWORD* msgAddress = (DWORD *)msgAdd;
	DWORD wxidAdd = (*msgAddress + 0x40);
	DWORD wxid2Add = (*msgAddress + 0x150);
	DWORD messageAdd = (*msgAddress + 0x68);
	//TCHAR buff[0x8000] = { 0 };
	TCHAR POSTFIELDS[0x8000] = { 0 };
	if (*(LPVOID *)wxid2Add <= 0x0) {
		//swprintf_s(buff, L"wxid:%s 消息内容:%s \r \n", *((LPVOID *)wxidAdd), *((LPVOID *)messageAdd));
		swprintf_s(POSTFIELDS, L"wxid=%s&wsg=%s", *((LPVOID *)wxidAdd), (*((LPVOID *)messageAdd)));
		//swprintf_s(buff, L"ESI=%p wxid=%p wxid2=%p wxid2=%p\r\n", msgAdd, msgAdd - 0x1A0, *((LPVOID *)wxidAdd));
	}
	else {
		//swprintf_s(buff, L"群ID:%s 发送者ID:%s 消息内容:%s \r \n", *((LPVOID *)wxidAdd), *((LPVOID *)wxid2Add), *((LPVOID *)messageAdd));
		swprintf_s(POSTFIELDS, L"wxid=%s&wxgh=%s&wsg=%s", *((LPVOID *)wxidAdd), *((LPVOID *)wxid2Add), *((LPVOID *)messageAdd));
	}
	/*
	======================写入txt的============================
	FILE *fpRead = fopen(("read.txt"), ("a,ccs=UTF-8"));
	fwrite(buff, sizeof(TCHAR), wcslen(buff), fpRead);
	fclose(fpRead);
	*/


	/*
	======================HTTP发信息============================
	*/
	CURL *curl;
	CURLcode res;
	const char * sendData = UnicodeToChar(POSTFIELDS);
	struct curl_slist *http_header = NULL;
	curl = curl_easy_init();
	http_header = curl_slist_append(http_header, "Content-Type:application/x-www-form-urlencoded;charset=UTF-8");
	curl_easy_setopt(curl, CURLOPT_URL, POSTURL);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sendData);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_header);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_HEADER, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	res = curl_easy_perform(curl);
	curl_slist_free_all(http_header);//记得要释放
	curl_easy_cleanup(curl);

}


//跳转过来的函数 我们自己的
VOID __declspec(naked) HookF()
{
	__asm {
		mov cEax, eax
		mov cEcx, ecx
		mov cEdx, edx
		mov cEbx, ebx
		mov cEsp, esp
		mov cEbp, ebp
		mov cEsi, esi
		mov cEdi, edi

		pushad
		pushfd
	}
	//然后跳转到我们自己的处理函数 想干嘛干嘛
	//消息接收 3.0.0.57 WinAdd + 0x3BA682 cesi
	//消息接收 3.2.1.127 WinAdd + 0x3E1FDA cEdi
	printLog(cEdi);
	retAdd = WinAdd + 0x3E1FDA;
	__asm {
		popfd
		popad
		jmp retAdd
	}
}

VOID StartHook(DWORD hookAdd, LPVOID jmpAdd)
{
	BYTE JmpCode[HOOK_LEN] = { 0 };
	JmpCode[0] = 0xE9;
	*(DWORD *)&JmpCode[1] = (DWORD)jmpAdd - hookAdd - HOOK_LEN;
	if (ReadProcessMemory(hWHND, (LPVOID)hookAdd, backCode, HOOK_LEN, NULL) == 0) {
		return;
	}
	if (WriteProcessMemory(hWHND, (LPVOID)hookAdd, JmpCode, HOOK_LEN, NULL) == 0) {
		return;
	}
}

VOID HookWechatRead()
{
	//消息接收 3.0.0.57 0x3BA67D
	//消息接收 3.2.1.127 0x3E1FD5
	hookAdd = getModuleAddress() + 0x3E1FD5;
	hWHND = OpenProcess(PROCESS_ALL_ACCESS, NULL, GetCurrentProcessId());
	WinAdd = getModuleAddress();
	StartHook(hookAdd, &HookF);
}