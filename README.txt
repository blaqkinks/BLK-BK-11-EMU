������: make all.

���������� (Win32): gcc/MinGW (build 20030504) + SDL-1.2.7

�������� ���������

- ����� ���������� ���������� ("cpu.c").
��� �� ������ ������������� (cpu_init) � ��������� ���������� (cpu_service)

- �� ���������� ��������� ���������� �� ������������� ������� � �����������
�����������.

- ������������� ��������� ����� device_install (����������� � access.c).
��������� pdp_qmap ������ � defines.h � ������� ��������.

- ��������� ��������� ����������� ����� (install_cp) � main.c

- ����� ������� ������ ��������� (����� logf)

- �� ��������� �������� ����������� EMT � TRAP (� main.c)

- ��������� ����� �����:

	-k FILE:
	������� ������������� �� FILE ���� ����� EMT 6
	
	-d FILE:
	���������� � FILE ����� ����� EMT 16/20

	-x FILE:
	��������� � ��������� ��������� ���� FILE	(��. �-��� launch � main.c)

- ��������� �� ������ ������ ����!! (sdl_sound.c).

- ���, SDL-����������� ��������� �� ������ ���������� � sdl_input.c

- ������� �������� ���������� - ��. kbd.h � sdl_input.c

- ���, ��������� � fake_tape � fake_disk, ���������� � sim_tape � sim_disk ��������������.

- ������:
������ ������� �����������������, ��� �������� �������� �� defines.h, ���-��� ����
���������, ��� ��� ����� ������������� static, etc.

��� ��� ���������� �������:

- ��� ������� ����������� � ������� (scr.c), ���������� �������������� �����������.
�������, ���, ������������ � SDL � sdl_video.c

- �������� �������� ������ ����� ����� ������ �������� EMT 36

- ��������� ���� ����� �� logf, ����������� ������� �����������.

- �������� ��������� ���������. ���������� ��������� dev_init ��:
���������� init (����������� ������� � ������) � reset (������
��� ��� ������ ����). ����������� � access.c ����������� ���������
� �����������.

- ����������� �������� (��� ���������?)

- � ���������� ��� ���-��� ����� ��������. (��������, �������� EIS?)
��������� ������� ���������� (� ��������� ��������� �������� ������� ������...)

- ����������� ������ ui.c ����������� ������������� �������-��������.
���� � �������, ��...

- �������, ��������� �������� Terak � ����������, ��� ��� �����.)

�� �, ����������, ������ ��� ������.

2018 - ������������ ����� - ���� �������� (allynddudnikov@bk.ru)

Build: make all.

The compiler (Win32): gcc / MinGW (build 20030504) + SDL-1.2.7

MAJOR CHANGES

- New implementation of the processor ("cpu.c").
There is also initialzation (cpu_init) and interrupt handling (cpu_service)

- CPU activity monitoring.

- Device initialization via device_install (implemented in access.c).
The pdp_qmap structure now defines.h and is slightly modified.

- Installation of interception of control points (install_cp) in main.c

- New message output system (via logf)

- By default, the EMT and TRAP tracing is enabled (in main.c)

- Several new options:

-k FILE:
time to redirect from FILE input through EMT 6

-d FILE:
copy to FILE output via EMT 16/20

-x FILE:
download and run the tape file FILE (see the launch in main.c)

- Completely new sound  (sdl_sound.c).

- Everything SDL-specific associated with the input is moved to sdl_input.c

- The keyboard has been slightly changed - see kbd.h and sdl_input.c

- Everything related to fake_tape and fake_disk is moved to sim_tape and sim_disk, respectively.

- Little things:
many functions are prototyped, all unnecessary is rendered from defines.h, something there
added, everything that can be declared static, etc.

What's needed:

- How to deal with the screen (scr.c), try to optimize the refresh rate.
 Render still done with SDL in sdl_video.c

- Completing the tape emulator through direct interception of EMT 36

- Translate the entire output to logf, arrange the diagnostic system.

- Complete the device interface. It is desirable to divide dev_init into:
actually init (executed once in the beginning) 
and reset (each time when the bus is reset). 
Implementing trace to access.c to the devices.

- Implement joystick (or joysticks?)

- Generally improve the CPU emulation. (Possibly EIS?)

-Check the timing of the instructions (in the Kalmyk emulator the clockrates? are slightly different.)

- Try instead of ui.c to implement a full-screen monitor-debugger.
It would be great, but ...

- Finally, make Terak work and see what it is. ;)

Well, of course, to finish all the mistakes ...

2018 - PUBLIC DOMAIN - Allynd Dudnikov (allynddudnikov@bk.ru)
