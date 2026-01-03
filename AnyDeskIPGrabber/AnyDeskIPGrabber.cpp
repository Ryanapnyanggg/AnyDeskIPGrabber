#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include <winsock2.h>
#include <iphlpapi.h>

std::string buscarLogAnyDesk() {
    std::vector<std::string> caminhos;
    caminhos.push_back(std::string(getenv("APPDATA")) + "\\AnyDesk");
    caminhos.push_back(std::string(getenv("PROGRAMDATA")) + "\\AnyDesk");
    caminhos.push_back("C:\\Program Files (x86)\\AnyDesk");
    caminhos.push_back("C:\\Program Files\\AnyDesk");
    caminhos.push_back(std::string(getenv("LOCALAPPDATA")) + "\\AnyDesk");

    std::string ipEncontrado = "";

    for (const auto& caminho : caminhos) {
        WIN32_FIND_DATAA dadosArquivo;
        HANDLE hFind = FindFirstFileA((caminho + "\\*").c_str(), &dadosArquivo);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::string nomeArquivo = dadosArquivo.cFileName;
                if (nomeArquivo.find(".trace") != std::string::npos ||
                    nomeArquivo.find("log") != std::string::npos ||
                    nomeArquivo.find(".log") != std::string::npos) {

                    std::string caminhoCompleto = caminho + "\\" + nomeArquivo;
                    std::ifstream arquivo(caminhoCompleto.c_str());

                    if (arquivo.is_open()) {
                        std::string linha;
                        while (std::getline(arquivo, linha)) {
                            if (linha.find("incoming") != std::string::npos ||
                                linha.find("connected") != std::string::npos ||
                                linha.find("connection") != std::string::npos) {

                                size_t posIP = linha.find_first_of("0123456789");
                                while (posIP != std::string::npos) {
                                    size_t fimIP = linha.find_first_not_of("0123456789.", posIP);
                                    std::string possivelIP = linha.substr(posIP, fimIP - posIP);

                                    int pontos = 0;
                                    for (char c : possivelIP) if (c == '.') pontos++;

                                    if (pontos == 3) {
                                        ipEncontrado = possivelIP;
                                        arquivo.close();
                                        return ipEncontrado;
                                    }
                                    posIP = linha.find_first_of("0123456789", fimIP);
                                }
                            }
                        }
                        arquivo.close();
                    }
                }
            } while (FindNextFileA(hFind, &dadosArquivo));
            FindClose(hFind);
        }
    }

    return ipEncontrado;
}

std::string buscarIPConexao() {
    std::string ip = "";
    DWORD tamanho = 0;

    if (GetExtendedTcpTable(NULL, &tamanho, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) != ERROR_INSUFFICIENT_BUFFER) {
        return "";
    }

    PMIB_TCPTABLE_OWNER_PID tabelaTCP = (PMIB_TCPTABLE_OWNER_PID)malloc(tamanho);
    if (!tabelaTCP) return "";

    if (GetExtendedTcpTable(tabelaTCP, &tamanho, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        for (DWORD i = 0; i < tabelaTCP->dwNumEntries; i++) {
            HANDLE processo = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, tabelaTCP->table[i].dwOwningPid);
            if (processo) {
                char caminho[MAX_PATH] = { 0 };
                if (GetModuleFileNameExA(processo, NULL, caminho, MAX_PATH)) {
                    std::string caminhoStr(caminho);
                    if (caminhoStr.find("anydesk.exe") != std::string::npos ||
                        caminhoStr.find("AnyDesk.exe") != std::string::npos) {

                        IN_ADDR addr;
                        addr.S_un.S_addr = tabelaTCP->table[i].dwRemoteAddr;
                        char* ipStr = inet_ntoa(addr);
                        if (ipStr && std::string(ipStr) != "0.0.0.0") {
                            ip = ipStr;
                            CloseHandle(processo);
                            break;
                        }
                    }
                }
                CloseHandle(processo);
            }
        }
    }

    free(tabelaTCP);
    return ip;
}

void listarConexoesAtivas() {
    DWORD tamanho = 0;

    if (GetExtendedTcpTable(NULL, &tamanho, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) != ERROR_INSUFFICIENT_BUFFER) {
        return;
    }

    PMIB_TCPTABLE_OWNER_PID tabelaTCP = (PMIB_TCPTABLE_OWNER_PID)malloc(tamanho);
    if (!tabelaTCP) return;

    if (GetExtendedTcpTable(tabelaTCP, &tamanho, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        std::cout << "\n=== CONEXOES ATIVAS ===\n";
        for (DWORD i = 0; i < tabelaTCP->dwNumEntries; i++) {
            IN_ADDR addrLocal, addrRemote;
            addrLocal.S_un.S_addr = tabelaTCP->table[i].dwLocalAddr;
            addrRemote.S_un.S_addr = tabelaTCP->table[i].dwRemoteAddr;

            char* ipLocal = inet_ntoa(addrLocal);
            char* ipRemoto = inet_ntoa(addrRemote);

            HANDLE processo = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, tabelaTCP->table[i].dwOwningPid);
            std::string nomeProcesso = "Desconhecido";

            if (processo) {
                char caminho[MAX_PATH] = { 0 };
                if (GetModuleFileNameExA(processo, NULL, caminho, MAX_PATH)) {
                    nomeProcesso = caminho;
                    size_t pos = nomeProcesso.find_last_of("\\");
                    if (pos != std::string::npos) {
                        nomeProcesso = nomeProcesso.substr(pos + 1);
                    }
                }
                CloseHandle(processo);
            }

            if (nomeProcesso.find("anydesk") != std::string::npos ||
                nomeProcesso.find("AnyDesk") != std::string::npos) {
                std::cout << "[ANYDESK] ";
            }

            std::cout << "Local: " << ipLocal << ":" << ntohs((u_short)tabelaTCP->table[i].dwLocalPort)
                << " -> Remoto: " << ipRemoto << ":" << ntohs((u_short)tabelaTCP->table[i].dwRemotePort)
                << " | PID: " << tabelaTCP->table[i].dwOwningPid
                << " | Processo: " << nomeProcesso << "\n";
        }
        std::cout << "=======================\n";
    }

    free(tabelaTCP);
}

int main() {
    SetConsoleTitle("AnyDesk IP Detector");
    system("cls");

    std::cout << "=========================================\n";
    std::cout << "    DETECTOR DE IP ANYDESK - ATIVO\n";
    std::cout << "=========================================\n\n";

    std::string ultimoIP = "";
    int tentativas = 0;

    while (true) {
        std::cout << "Verificando conexoes AnyDesk... (" << ++tentativas << ")\n";

        std::string ipLog = buscarLogAnyDesk();
        std::string ipConexao = buscarIPConexao();

        if (!ipLog.empty() && ipLog != ultimoIP) {
            ultimoIP = ipLog;
            std::cout << "\n[!] IP ENCONTRADO NO LOG: " << ipLog << "\n\n";
            listarConexoesAtivas();
        }

        if (!ipConexao.empty() && ipConexao != ultimoIP) {
            ultimoIP = ipConexao;
            std::cout << "\n[!] CONEXAO ATIVA DETECTADA: " << ipConexao << "\n\n";
            listarConexoesAtivas();
        }

        if (ipLog.empty() && ipConexao.empty() && !ultimoIP.empty()) {
            std::cout << "\n[i] Conexao encerrada. Aguardando nova...\n\n";
            ultimoIP = "";
        }

        Sleep(3000);
        system("cls");
        std::cout << "=========================================\n";
        std::cout << "    DETECTOR DE IP ANYDESK - ATIVO\n";
        std::cout << "=========================================\n\n";
    }

    return 0;
}