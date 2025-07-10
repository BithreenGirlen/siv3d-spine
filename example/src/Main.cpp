
#include <locale.h>

#include "siv3d_main_window.h"

void Main()
{
	::setlocale(LC_ALL, ".utf8");

	CSiv3dMainWindow mainWindow(U"Siv3D-Spine test");

	mainWindow.Display();
}
