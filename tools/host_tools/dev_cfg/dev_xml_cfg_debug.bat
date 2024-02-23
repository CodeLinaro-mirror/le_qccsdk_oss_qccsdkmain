chdir /d ..\..\..\..\tools\Target_tools\dev_cfg\export\
attrib -r ..\..\..\..\core\common\nt_devcfg.h /s   
@echo off
python --version 2>NUL
if errorlevel 1 goto PYTHON_DOES_NOT_EXIST

:: code to make the master xml
del all_xml_modules_to_cat.xml
python xml_combine_debug.py ?.xml > all_xml_modules_to_cat.xml
del master_xml.xml
type first_part_to_cat.xml >> master_xml.xml
type all_xml_modules_to_cat.xml >> master_xml.xml
type last_part_to_cat.xml >> master_xml.xml
del ..\inc\master_xml.xml
type master_xml.xml >> ..\inc\master_xml.xml

:: code to generate the two header files of all the instance enum form in nt_devcfg.h and instance in uint32 form in nt_devcfg_structure.h
:: code to generate the two header files of all the byte instance enum form in byte_enum.h and instance in uint32 form in byte_struct.h
setlocal enabledelayedexpansion
set input="master_xml.xml"
set byte_seq_input="..\inc\byte_sequence.xml"
set output="Names.h"
set output2="Names2.h"
set byte_enum_output="byte_enum.h"
set byte_struct_output="byte_struct.h"
set /a count = 0
set /a count1 = 0
if exist %output% del %output%
if exist %output2% del %output2%
if exist %byte_enum_output% del %byte_enum_output%
if exist %byte_struct_output% del %byte_struct_output%
setlocal enabledelayedexpansion
echo /** System Generated File  >> Names.h
echo /** System Generated File  >> Names2.h
echo /** System Generated File  >> byte_enum.h
echo /** System Generated File  >> byte_struct.h
echo   * Cann't Change Manually */  >> Names.h
echo   * Cann't Change Manually */  >> Names2.h
echo   * Cann't Change Manually */  >> byte_enum.h
echo   * Cann't Change Manually */  >> byte_struct.h

echo #ifndef CORE_DEV_CFG_EXPORT_NT_DEVCFG_H_ >> Names.h
echo #define CORE_DEV_CFG_EXPORT_NT_DEVCFG_H_ >> Names.h
echo typedef enum nt_devcfg_id_s >> Names.h
echo {>> Names.h

echo #ifndef CORE_COMMON_NT_DEVCFG_BYTE_SEQ_H_ >> byte_enum.h
echo #define CORE_COMMON_NT_DEVCFG_BYTE_SEQ_H_ >> byte_enum.h
echo typedef enum nt_devcfg_byte_seq_id_s >> byte_enum.h
echo {>> byte_enum.h

echo #ifndef CORE_DEV_CFG_INC_NT_DEVCFG_STRUCTURE_H_ >> Names2.h
echo #define CORE_DEV_CFG_INC_NT_DEVCFG_STRUCTURE_H_ >> Names2.h
echo #include "nt_devcfg_def.h">> Names2.h
echo #include "nt_devcfg_types.h" >> Names2.h
echo void nt_devcfg_parse();      >> Names2.h
echo typedef struct nt_devcfg_structure_s >> Names2.h
echo {>> Names2.h

echo #ifndef CORE_DEV_CFG_INC_NT_DEVCFG_BYTE_SEQ_STRUCTURE_H_ >> byte_struct.h
echo #define CORE_DEV_CFG_INC_NT_DEVCFG_BYTE_SEQ_STRUCTURE_H_ >> byte_struct.h
echo #include "nt_devcfg_def.h">> byte_struct.h
echo #include "nt_devcfg_types.h" >> byte_struct.h
echo void nt_devcfg_byte_seq_parse();      >> byte_struct.h
echo typedef struct nt_devcfg_byte_seq_structure_s >> byte_struct.h
echo {>> byte_struct.h

setlocal disableDelayedExpansion
for /f "delims=" %%A in ('findstr /n /c:"id_name=" %input%') do (
  set "ln=%%A"
  set /a count += 1
  setlocal enableDelayedExpansion
  call :parseLine
  endlocal
)

echo }nt_devcfg_id_t; >> Names.h
echo void* nt_devcfg_get_config(int enum_id);		//call back function for uint value >> Names.h
echo void* nt_devcfg_get_ascii_config(int enum_id);	// call back function for ascii value >> Names.h
echo #endif /* CORE_DEV_CFG_EXPORT_NT_DEVCFG_H_ */ >> Names.h
del ..\..\..\..\core\common\nt_devcfg.h
type Names.h >> ..\..\..\..\core\common\nt_devcfg.h

echo }nt_devcfg_structure_t; >> Names2.h
echo #endif /* CORE_DEV_CFG_INC_NT_DEVCFG_STRUCTURE_H_ */ >> Names2.h
del ..\inc\nt_devcfg_structure.h
type Names2.h >> ..\inc\nt_devcfg_structure.h

for /f "delims=" %%A in ('findstr /n /c:"id_name=" %byte_seq_input%') do (
  set "ln=%%A"
  set /a count1 += 1
  setlocal enableDelayedExpansion
  call :parseLinebyteseq
  endlocal
)

echo }nt_devcfg_byte_seq_id_t; >> byte_enum.h
echo void* nt_devcfg_get_byte_seq_config(int enum_id);		//call back function for byte sequence value >> byte_enum.h
echo #endif /* CORE_COMMON_NT_DEVCFG_BYTE_SEQ_H_ */ >> byte_enum.h
del ..\..\..\..\core\common\nt_devcfg_byte_seq.h
type byte_enum.h >> ..\..\..\..\core\common\nt_devcfg_byte_seq.h

echo }nt_devcfg_byte_seq_structure_t; >> byte_struct.h
echo #endif /* CORE_DEV_CFG_INC_NT_DEVCFG_BYTE_SEQ_STRUCTURE_H_ */ >> byte_struct.h
del ..\inc\nt_devcfg_byte_seq_structure.h
type byte_struct.h >> ..\inc\nt_devcfg_byte_seq_structure.h
exit /b

:parseLine
set "ln2=!ln:*id_name=!"
if "!ln2!"=="!ln!" exit /b
for /f tokens^=2^ delims^=^" %%B in ("!ln2!") do (
  setlocal disableDelayedExpansion
  >>%output% echo(%%B = %count%,
  endlocal
)
for /f tokens^=2^ delims^=^" %%B in ("!ln2!") do (
  setlocal disableDelayedExpansion
  >>%output2% echo(uint32 %%B ;
  endlocal
)
set "ln=!ln2!"
goto :parseLine
::goto :EOF

:: code for byte sequence
:parseLinebyteseq
set "ln2=!ln:*id_name=!"
if "!ln2!"=="!ln!" exit /b
for /f tokens^=2^ delims^=^" %%B in ("!ln2!") do (
  setlocal disableDelayedExpansion
  >>%byte_enum_output% echo(%%B = %count1%,
  endlocal
)
for /f tokens^=2^ delims^=^" %%B in ("!ln2!") do (
  setlocal disableDelayedExpansion
  >>%byte_struct_output% echo(uint32 %%B ;
  endlocal
)
set "ln=!ln2!"
goto :parseLinebyteseq
goto :EOF

:PYTHON_DOES_NOT_EXIST
echo Python is not installed in this system.
goto :EOF