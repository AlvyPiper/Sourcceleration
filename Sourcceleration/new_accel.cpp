#include "interception.h"
#include "utils.h"
#include <windows.h>
#include <cmath>
#include <math.h>
#include <iostream>

#define msg printf

int main()
{
	InterceptionContext context;
	InterceptionDevice device;
	InterceptionStroke stroke;

	raise_process_priority();

	context = interception_create_context();

	interception_set_filter(context, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_MOVE);


	double
		frametimems,
		mouse_movement_distance_squared,
		newmovement,
		counts,
		exponential_accel,
		accelerationtype = 0,
		accelerationscale = 0,
		sensitivitycap = 0,
		sensitivity = 0,
		exponent_num = 0,
		counttype = 0,
		remainderx = 0,
		remaindery = 0,
		settingvalue,
		mx,
		my;

	char settingname[24];

	COORD coord;

	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);


	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof cfi;
	cfi.nFont = 0;
	cfi.dwFontSize.X = 0;
	cfi.dwFontSize.Y = 14;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	wcscpy(cfi.FaceName, L"Consolas");
	SetCurrentConsoleFontEx(hConsole, FALSE, &cfi);


	coord.X = 80;
	coord.Y = 25;
	SetConsoleScreenBufferSize(hConsole, coord);

	SMALL_RECT conSize;

	conSize.Left = 0;
	conSize.Top = 0;
	conSize.Right = coord.X - 1;
	conSize.Bottom = coord.Y - 1;

	SetConsoleWindowInfo(hConsole, TRUE, &conSize);

	FILE *fp;

	if ((fp = fopen(".\\settings.txt", "r+")) == NULL) {
		SetConsoleTextAttribute(hConsole, 0x04);
		printf("* Cannot read from settings file. Using defaults.\n");
		exponent_num = 1.05;
		accelerationscale = 0.64;
		sensitivity = 1;
		counttype = 1;
		sensitivitycap = 2;
		accelerationtype = 1;
		SetConsoleTextAttribute(hConsole, 0x08);
	}
	else
	{
		for (int i = 0; i < 99 && fscanf(fp, "%s = %lf", &settingname, &settingvalue) != EOF; i++) {
			if (strcmp(settingname, "Sensitivity") == 0)
			{
				sensitivity = settingvalue;
			}
			else if (strcmp(settingname, "SensitivityCap") == 0)
			{
				sensitivitycap = settingvalue;
			}
			else if (strcmp(settingname, "ExponentAmt") == 0)
			{
				exponent_num = settingvalue;
			}
			else if (strcmp(settingname, "CountType") == 0)
			{
				counttype = settingvalue;
			}
			else if (strcmp(settingname, "AccelerationType") == 0)
			{
				accelerationtype = settingvalue;
			}
			else if (strcmp(settingname, "AccelerationScale") == 0)
			{
				accelerationscale = settingvalue;
			}
			else
			{
				SetConsoleTextAttribute(hConsole, 0x04);
				msg("* Something is wrong with your config file.\n");
				SetConsoleTextAttribute(hConsole, 0x08);
			}
		}
	}
	SetConsoleTextAttribute(hConsole, 0x02);
	msg("alya's source acceleration emulator\nusing code created by povohat and francisco lopes\n\n");
	SetConsoleTextAttribute(hConsole, 0x08);
	printf("Sensitivity: %f\nSensitivityCap: %f\nExponentAmt: %f\nCountType: %f\nAccelerationType: %f\nAccelerationScale %f\n", sensitivity, sensitivitycap, exponent_num, counttype, accelerationtype, accelerationscale);

	while (interception_receive(context, device = interception_wait(context), &stroke, 1) > 0)
	{

		if (interception_is_mouse(device))
		{
			InterceptionMouseStroke &mstroke = *(InterceptionMouseStroke *)&stroke;

			if (!(mstroke.flags & INTERCEPTION_MOUSE_MOVE_ABSOLUTE)) {

				mx = (double) mstroke.x;
				my = (double) mstroke.y;

				if (counttype == 1)
				{
					counts = sqrt(mx * mx + my * my); //allow for linear (sqrt(mx * mx)) = mx
				}
				else if (counttype == 2)
				{
					counts = mx * mx + my * my; //allow for hyperbola (mx ^ 2)
				}
				else if (counttype > 2 || counttype < 0) //BUG: Mouse movement is wrong on the first count if counttype is above 3 or less than 0.
				{
					counttype = 1;
				}

				if (counts > 0 && accelerationtype == 1)
				{
					exponential_accel = max(0.0f, (exponent_num - 1.0f) / 2.0f); //returns largest of (0.0f) and (m_customaccel_exponent - 1.0f) / 2.0f) if both are the same, 0.0f is returned.
					newmovement = powf(counts, exponential_accel) * sensitivity; //power of float (movement distance in hyperbola, result of exponent) * mouse_sensitivity;
				}
				else  if (counts > 0 && accelerationtype == 2)
				{
					newmovement = (powf(counts, exponent_num) * accelerationscale + sensitivity); //((power of float(distance of linear movement, exponent)) * scale + mouse_sensitivity)
				}
				else if (accelerationtype > 2 || accelerationtype < 1)
				{
					accelerationtype = 1;
				}

				if (newmovement > sensitivitycap)
				{
					newmovement = sensitivitycap;
				}

				mx *= newmovement; //multiply movement by accelerated sensitivity on pitch
				my *= newmovement; //multiply movement by accelerated sensitivity on yaw

				// hold the remainder and pass it into the next count or else we will get an unusual upward and left movement. CREDITS: Povohat
				mx += remainderx;
				my += remaindery;
				
				remainderx = mx - floor(mx);
				remaindery = my - floor(my);

				mstroke.x = (int) floor(mx);
				mstroke.y = (int) floor(my);
			}

			interception_send(context, device, &stroke, 1);
		} 
	}

	interception_destroy_context(context);

	return 0;
}
