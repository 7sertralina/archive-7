#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

struct AuthStruct {
	const char* Titulo;
	std::vector<const char*> Colluns;
	bool OverlayView;
	bool AtivarFuncoes;
	bool Progress;
	bool Attached;
	bool Autenticado;
	char Usuario[256];
	char Senha[256];       // License Key
	char Password[256];    // Login password
	char HWID[256];
	char DiscordID[64];    // Username
	char DiscordAvatarUrl[512];
	int dias_restantes;
	std::string SessionToken;
	std::string LoaderHash;
	bool RememberMe;
};
extern AuthStruct Auth;
