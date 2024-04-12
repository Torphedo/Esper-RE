@echo off
for %%A IN (bin\*.alr) DO (
    start alr --dump %%A
)

