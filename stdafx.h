// stdafx.h: ���������� ���� ��� ����������� ��������� ���������� ������
// ��� ���������� ������ ��� ����������� �������, ������� ����� ������������, ��
// �� ����� ����������
//

#pragma once

#include "targetver.h"

#ifdef _WIN32
#include <windows.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void LogPrintf( bool iserror, char *fmt, ... );

// TODO. ���������� ����� ������ �� �������������� ���������, ����������� ��� ���������