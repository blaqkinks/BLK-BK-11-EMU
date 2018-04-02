Сборка: make all.

Компилятор (Win32): gcc/MinGW (build 20030504) + SDL-1.2.7

ОСНОВНЫЕ ИЗМЕНЕНИЯ

- Новая реализация процессора ("cpu.c").
Там же теперь инициализация (cpu_init) и обработка прерываний (cpu_service)

- По заверщении выводится статистика по процессорному времени и выполненным
инструкциям.

- Инициализация устройств через device_install (реализовано в access.c).
Структура pdp_qmap теперь в defines.h и немного изменена.

- Установка перехвата контрольных точек (install_cp) в main.c

- Новая система вывода сообщений (через logf)

- По умолчанию включена трассировка EMT и TRAP (в main.c)

- Несколько новых опций:

	-k FILE:
	времено перенаправить из FILE ввод через EMT 6
	
	-d FILE:
	копировать в FILE вывод через EMT 16/20

	-x FILE:
	загрузить и запустить ленточный файл FILE	(см. ф-цию launch в main.c)

- Полностью по новому сделан звук!! (sdl_sound.c).

- Все, SDL-специфичное связанное со вводом перенесено в sdl_input.c

- Немного изменена клавиатура - см. kbd.h и sdl_input.c

- Все, связанное с fake_tape и fake_disk, перенесено в sim_tape и sim_disk соответственно.

- Мелочи:
многие функции прототипизированы, все ненужное вынесено из defines.h, кое-что туда
добавлено, все что можно декларировано static, etc.

ЧТО ЕЩЕ ЖЕЛАТЕЛЬНО СДЕЛАТЬ:

- Как следует разобраться с экраном (scr.c), попытаться оптимизировать регенерацию.
Вынести, все, относящнееся к SDL в sdl_video.c

- Доделать эмуляцию чтения ленты через прямой перехват EMT 36

- Перевести весь вывод на logf, упорядочить систему диагностики.

- Доделать интерфейс устройств. Желательно разделить dev_init на:
собственно init (выполняется однажды в начале) и reset (каждый
раз при сбросе шины). Реализовать в access.c трассировку обращения
к устройствам.

- Реализовать джойстик (или джойстики?)

- В процессоре еще кое-что можно улучшить. (Возможно, добавить EIS?)
Проверить тайминг инструкций (в эмуляторе Калмыкова значения немного другие...)

- Попробывать вместо ui.c реализовать полноэкранный монитор-отладчик.
Было б здорово, но...

- Наконец, заставить работать Terak и посмотреть, что это такое.)

Ну и, разумеется, добить все ошибки.

2018 - ОБЩЕСТВЕННЫЙ ДОМЕН - Олин Дудников (allynddudnikov@bk.ru)

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
