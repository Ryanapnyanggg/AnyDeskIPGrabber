#ifndef MONITOR_H
#define MONITOR_H

#include <string>

class Monitor {
public:
    void start();

private:
    void showConnections();
    void checkPorts();
    void findProcess();
    void showIPs();
    void clearScreen();
    void extractIP(const std::string& line, std::string& ip);
};

#endif