all: svc winkey


svc.obj: svc.cpp
	cl /Wall /WX -c svc.cpp

winkey.obj: winkey.cpp
	cl /Wall /WX -c winkey.cpp

svc: svc.obj
	link svc.obj

winkey: winkey.obj
	link winkey.obj

clean:
	del winkey.obj svc.obj

fclean: clean
	del winkey.exe svc.exe