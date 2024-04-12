@echo off
for %%A IN (bin\*.alr) DO (
    start /b alr --dump %%A > nul
)

