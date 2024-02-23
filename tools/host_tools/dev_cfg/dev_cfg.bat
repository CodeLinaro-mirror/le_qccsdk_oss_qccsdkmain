chdir /d ..\..\..\..\tools\Target_tools\dev_cfg\inc\

@echo off
python --version 2>NUL
if errorlevel 1 goto PYTHON_DOES_NOT_EXIST

for /f "delims=" %%V in ('python -V') do @set ver=%%V
echo  %ver% is installed...
python propgen.py --XmlFile=master_xml.xml --DirName=%cd% --ConfigFile=nt_devcfg_from_master_xml.h --DevcfgDataFile=nt_devcfg_data.h --ConfigType=devcfg_xml
python propgen.py --XmlFile=byte_sequence.xml --DirName=%cd% --ConfigFile=nt_devcfg_from_byte_sequence.h --DevcfgDataFile=nt_devcfg_byte_data.h --ConfigType=devcfg_byte_seq_xml
goto :EOF

:PYTHON_DOES_NOT_EXIST
echo Python is not installed in this system.
goto :EOF
