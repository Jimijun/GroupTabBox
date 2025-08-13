#pragma once

#include <string>

std::wstring programPath();
std::wstring programDir();

void setAutoStart(const wchar_t *reg_key, bool enable);

bool isAlreadyAdmin();

void runAsAdmin();
