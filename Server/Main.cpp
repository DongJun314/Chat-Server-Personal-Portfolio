#include "Headers.h"
#include "Server.h"

using namespace std;

int main(void)
{
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	Server server("127.0.0.1", 30104, 8);
	if (!server.Run()) 
	{
		cerr << "Server failed to start." << endl;
		return 1;
	}
	return 0;
}