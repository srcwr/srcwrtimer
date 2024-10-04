#include <srcwr/sample>

public void OnPluginStart()
{
	RegConsoleCmd("sm_winfo", Command_Winfo);
}

Action Command_Winfo(int client, int args)
{
	char buf[256];
	Sample_GetWindowsInfo(buf, sizeof(buf));
	PrintToChatAll("%s", buf);
	PrintToServer("%s", buf);
	return Plugin_Handled;
}
