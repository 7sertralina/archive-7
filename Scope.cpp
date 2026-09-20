#include "Scope.h"
#include <cstring>

AuthStruct Auth;

namespace {
	struct AuthInitializer {
		AuthInitializer() {
			Auth.Titulo = "Safety Softwares";
			Auth.Colluns = { "Main", "Visuals", "Misc" };
			Auth.OverlayView = true;
			Auth.AtivarFuncoes = true;
			Auth.Progress = false;
			Auth.Attached = false;
			Auth.Autenticado = false;
			Auth.dias_restantes = 0;
			memset(Auth.Usuario, 0, sizeof(Auth.Usuario));
			memset(Auth.Senha, 0, sizeof(Auth.Senha));
			memset(Auth.HWID, 0, sizeof(Auth.HWID));
			memset(Auth.DiscordID, 0, sizeof(Auth.DiscordID));
			memset(Auth.DiscordAvatarUrl, 0, sizeof(Auth.DiscordAvatarUrl));
			Auth.SessionToken = "";
			Auth.LoaderHash = "safetysoftwares";
			Auth.RememberMe = false;
		}
	};
	static AuthInitializer authInit;
}

