# Tinky-Winkey, Windows Service and Keylogger

## Introduction

Windows services are program that operate in background. They are often managed
by the Service Control Manager. Services interact with the SCM through the
Windows API or Windows service management tools such as sc.exe.
note: terminating sc.exe is used as a method of causing the Blue Screen of Death.

Keyloggers are programs that track the activities of a keyboard. Keyloggers are a
form of spyware where users are unaware their actions are being tracked. Keyloggers
can be used for a variety of purposes; hackers may use them to maliciously gain
access to your private information, while employers might use them to monitor
employee activities.

## Compilation

`nmake {all,re,clean,fclean}` at the root directory of the project. [note: cl.exe is required for compilation (Installed via Visual Studio)]

## Usage

svc.exe and winkey.exe must be in the same directory. Then running as administrator:

```
.\svc.exe install # Install the service

.\svc.exe start   # Start the service and run the keylogger in background

.\svc.exe stop    # Stop the service and the keylogger

.\svc.exe delete  # delete the service
```

Logs will be stored in `C:\Windows\logs.txt`

## Additional Notes

Lightweight Windows Image: https://developer.microsoft.com/en-us/microsoft-edge/tools/vms/

Visual Studio: https://visualstudio.microsoft.com/downloads/

SSH install: https://learn.microsoft.com/en-us/windows-server/administration/openssh/openssh_install_firstuse

## Collaboration

Project made in collaboration with [idosdool](https://github.com/idosdool)
