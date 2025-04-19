#pragma once

#include <string>

bool drivesStart(const std::string &fdd, const std::string &hdd);
void drivesPollInt13(void);
