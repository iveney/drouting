@echo off

FOR /F %%v IN ('dir /d /b *_sol.tex') DO d:\tools\ctex\texmf\miktex\bin\pdflatex.exe %%v
