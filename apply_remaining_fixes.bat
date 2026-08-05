@echo off
echo Applying remaining fixes...

REM Fix BankVersion.cpp: it->second -> it.value()
copy /Y "src\core\FileFormat\BankVersion_tmp.cpp" "src\core\FileFormat\BankVersion.cpp"
if %ERRORLEVEL% EQU 0 (
    echo [OK] BankVersion.cpp fixed
    del "src\core\FileFormat\BankVersion_tmp.cpp"
) else (
    echo [FAIL] BankVersion.cpp still locked - run this script after closing all editors
)

REM Fix AudioTypes.cpp: KSBankWriter -> fileformat::KSBankWriter
python -c "
import re
path = r'src\core\Audio\AudioTypes.cpp'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()
content = content.replace('    KSBankWriter writer;', '    fileformat::KSBankWriter writer;')
with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print('[OK] AudioTypes.cpp fixed')
"

echo Done.
pause
