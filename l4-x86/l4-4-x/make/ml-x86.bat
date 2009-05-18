@echo off
@ml /Dtarget=x86 /c /Sl124 /Sp109 /Fl /nologo /Sn /I..\kernifc /I..\userifc ..\kernel\%1.%2

