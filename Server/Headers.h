#pragma once

// C++ 표준 라이브러리
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>
#include <vector>
#include <array>
#include <unordered_set>

// C 스타일 I/O (필요하다면)
#include <cstdio>

// Windows 전용 네트워크 헤더
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
