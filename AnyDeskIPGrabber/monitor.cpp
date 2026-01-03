#define _CRT_SECURE_NO_WARNINGS
#include "monitor.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

void Monitor::clearScreen() {
    system("cls");
}

void Monitor::extractIP(const std::string& line, std::string& ip) {
    size_t start = line.find_last_of(' ');
    if (start == std::string::npos) return;

    start = line.find_last_of(' ', start - 1);
    if (start == std::string::npos) return;

    start++;
    size_t end = line.find(':', start);
    if (end == std::string::npos) return;

    ip = line.substr(start, end - start);
}

void Monitor::showConnections() {
    system("netstat -an > temp_net.txt");

    FILE* fp = fopen("temp_net.txt", "r");
    if (!fp) return;

    char line[1000];
    int count = 0;

    printf("Con. ESTABLISHED:\n");
    printf("=====================\n\n");

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "ESTABLISHED")) {
            printf("%s", line);

            std::string ip;
            extractIP(line, ip);

            if (!ip.empty()) {
                printf("   -> IP: %s\n", ip.c_str());
            }

            count++;
        }
    }

    fclose(fp);
    remove("temp_net.txt");

    printf("\nTotal: %d conexoes\n", count);
}

void Monitor::checkPorts() {
    printf("\nPort AnyDesk:\n");
    printf("===============\n\n");

    int ports[] = { 7070, 6568, 7071, 6569 };

    for (int i = 0; i < 4; i++) {
        char cmd[100];
        sprintf(cmd, "netstat -an | findstr :%d", ports[i]);

        FILE* fp = _popen(cmd, "r");
        if (fp) {
            char line[256];
            int found = 0;

            while (fgets(line, sizeof(line), fp)) {
                if (!found) {
                    printf("Porta %d:\n", ports[i]);
                    found = 1;
                }
                printf("   %s", line);
            }

            _pclose(fp);

            if (!found) {
                printf("Porta %d: Nenhuma\n", ports[i]);
            }
        }
    }
}

void Monitor::findProcess() {
    printf("\nProcess AnyDesk:\n");
    printf("=================\n");
    system("tasklist | findstr /i anydesk");
}

void Monitor::showIPs() {
    printf("\nIPs:\n");
    printf("============\n");
    system("netstat -an | findstr ESTABLISHED | findstr /v \"127.0.0.1\"");
}

void Monitor::start() {
    system("chcp 65001 >nul");
    system("title AnyDesk Monitor");

    while (true) {
        clearScreen();

        printf("=== ANYDESK MONITOR ===\n\n");

        showConnections();
        checkPorts();
        findProcess();
        showIPs();

        printf("\n=======================\n");
        printf("Atualizando...\n");
        Sleep(10000);
    }
}