#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream> 
#include <cstdio> 
#include <cstring>
#include <map>
#include <winsock2.h> 
#pragma comment(lib, "WS2_32.lib")
using namespace std;

//Rapidjson library
#include "document.h"
#include "writer.h"
#include "stringbuffer.h"
using namespace rapidjson;

map <string, string> dict;
map <string, string> :: iterator it, it_2;
SOCKET server;

Document makeJson(const char* s1, const char* s2, int flag = 0) {
	Document response;
	Value json_val;
	auto& allocator = response.GetAllocator();
	response.SetObject();
	json_val.SetString(s1, allocator);
	response.AddMember("status", json_val, allocator);
	if (!flag) {
		json_val.SetString(s2, allocator);
		response.AddMember("value", json_val, allocator);
	}
	else if (flag == 1) {
		json_val.SetString(s2, allocator);
		response.AddMember("description", json_val, allocator);
	}
	return response;
}

DWORD WINAPI serverReceive(LPVOID lpParam) { //Получение данных от клиента
	char buffer[1024] = { 0 }; //Буфер для данных
	SOCKET client = *(SOCKET*)lpParam; //Сокет для клиента
	if (recv(client, buffer, sizeof(buffer), 0) == SOCKET_ERROR) {
		//Если не удалось получить данные буфера, сообщить об ошибке и выйти
		cout << "recv function failed with error " << WSAGetLastError() << endl;
		return -1;
	}

	Document d, response;
	d.Parse(buffer);
	if (!d.IsObject()) {
		response = makeJson("error", "this request is not a JSON format", 1);
	}
	else {
		if (!d.HasMember("request")) {
			response = makeJson("error", "client must to make field 'request'", 1);
		}
		else {
			if (strcmp(d["request"].GetString(), "read") == 0) {
				if (!d.HasMember("key")) {
					response = makeJson("error", "client must to make field 'key'", 1);
				}
				else {
					it = dict.find(d["key"].GetString());
					if (it == dict.end()) {
						response = makeJson("error", "dict has no this key", 1);
					}
					else {
						response = makeJson("ok", it->second.c_str());
					}
				}
			}
			else if (strcmp(d["request"].GetString(), "write") == 0) {
				if (!d.HasMember("key")) {
					response = makeJson("error", "client must to make field 'key'", 1);
				}
				else if (!d.HasMember("value")) {
					response = makeJson("error", "client must to make field 'value'", 1);
				}
				else {
					dict.insert(make_pair(d["key"].GetString(), d["value"].GetString()));
					response = makeJson("ok", "", 2);
				}
			}
			else {
				response = makeJson("error", "this request is wrong, it can be 'read' or 'write'", 1);
			}
		}
	}
	StringBuffer buff;
	Writer<StringBuffer> writer(buff);
	response.Accept(writer);
	cout << buff.GetString() << endl;

	cout << "client disconnected" << endl;
	return 1;
}

DWORD WINAPI serverSend(LPVOID lpParam) { //Отправка данных клиенту
	char buffer[1024] = { 0 };
	SOCKET client = *(SOCKET*)lpParam;
	while (true) {
		fgets(buffer, 1024, stdin);
		if (send(client, buffer, sizeof(buffer), 0) == SOCKET_ERROR) {
			cout << "send failed with error " << WSAGetLastError() << endl;
			return -1;
		}
		if (strcmp(buffer, "exit\n") == 0) {
			cout << "Thank you for using the application" << endl;
			break;
		}
	}
	return 1;
}

DWORD WINAPI connectClient(LPVOID lpParam) {
	//Если соединение установлено
	cout << "Client connected!" << endl;

	DWORD tid; //Идентификатор
	SOCKET client = *(SOCKET*)lpParam; //Сокет для клиента
	HANDLE t1 = CreateThread(NULL, 0, serverReceive, &client, 0, &tid); //Создание потока для получения данных
	if (t1 == NULL) { //Ошибка создания потока
		cout << "Thread Creation Error: " << WSAGetLastError() << endl;
	}
	HANDLE t2 = CreateThread(NULL, 0, serverSend, &client, 0, &tid); //Создание потока для отправки данных
	if (t2 == NULL) {
		cout << "Thread Creation Error: " << WSAGetLastError() << endl;
	}

	WaitForSingleObject(t1, INFINITE);
	WaitForSingleObject(t2, INFINITE);

	closesocket(client); //Закрыть сокет
	if (closesocket(server) == SOCKET_ERROR) { //Ошибка закрытия сокета
		cout << "Close socket failed with error: " << WSAGetLastError() << endl;
		return -1;
	}
	WSACleanup();
}

int main() {
	WSADATA WSAData; //Данные 
	SOCKET client; //Сокеты сервера и клиента
	SOCKADDR_IN serverAddr, clientAddr; //Адреса сокетов
	WSAStartup(MAKEWORD(2, 0), &WSAData);
	server = socket(AF_INET, SOCK_STREAM, 0); //Создали сервер
	if (server == INVALID_SOCKET) {
		cout << "Socket creation failed with error:" << WSAGetLastError() << endl;
		return -1;
	}
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(5555);
	if (bind(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		cout << "Bind function failed with error: " << WSAGetLastError() << endl;
		return -1;
	}

	if (listen(server, 0) == SOCKET_ERROR) { //Если не удалось получить запрос
		cout << "Listen function failed with error:" << WSAGetLastError() << endl;
		return -1;
	}
	cout << "Listening for incoming connections...." << endl;

	char buffer[1024]; //Создать буфер для данных
	int clientAddrSize = sizeof(clientAddr); //Инициализировать адерс клиента
	for (;;) {
		if ((client = accept(server, (SOCKADDR*)&clientAddr, &clientAddrSize)) != INVALID_SOCKET) {
			//Если соединение установлено
			DWORD tid; //Идентификатор
			HANDLE t1 = CreateThread(NULL, 0, connectClient, &client, 0, &tid); //Создание потока для получения данных
		}
		else break;
	}
}
