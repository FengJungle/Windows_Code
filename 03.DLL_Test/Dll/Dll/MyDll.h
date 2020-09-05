#pragma once

#ifndef _MYDLL_H__
#define _MYDLL_H__

__declspec(dllexport) int sum(int a, int b);
__declspec(dllexport) int sub(int a, int b);
__declspec(dllexport) int mul(int a, int b);

int sum(int a, int b);

int sub(int a, int b);

int mul(int a, int b);

#endif // _MYDLL_H__
