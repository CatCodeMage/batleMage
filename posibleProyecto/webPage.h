#pragma once
#include <sstream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <map>
#include <string>
#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
#define CLOSE_SOCKET closesocket

const int PORT = 8080;
const std::string DESTINATION_FILE = "../db/combat.json";

std::string urlDecode(const std::string& s);

std::map<std::string, std::string> parseForm(const std::string& body);

std::string builPage();

void openBrowser(const std::string& url);

bool saveCombatMetadata(const std::map<std::string, std::string>& form);

bool sendForm();