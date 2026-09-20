#pragma once
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>
#include <iphlpapi.h>

#include <cstdlib>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "iphlpapi.lib")

#include <logs.h>
#include <console.h>
#include <utils.h>
#include <cleaner.h>
#include <network.h>
