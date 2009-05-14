@echo off

FOR /D %%v IN (DAC05 DAC05_2 DATE06 DATE06_3) DO cd %%v && gen.bat && cd ..
