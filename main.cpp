#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <sstream>
#include <cstdlib>
using namespace std;

bool verificaPrim(int valoare) {
    if (valoare < 2) return false;
    for (int divizor = 2; divizor * divizor <= valoare; ++divizor) {
        if (valoare % divizor == 0) return false;
    }
    return true;
}

void proceseazaNumere(int limitaInferioara, int limitaSuperioara, HANDLE canalScriere) {
    DWORD bytesScrise;
    for (int numar = limitaInferioara; numar <= limitaSuperioara; ++numar) {
        if (verificaPrim(numar)) {
            WriteFile(canalScriere, &numar, sizeof(numar), &bytesScrise, NULL);
        }
    }
    int semnalFinal = -1;
    WriteFile(canalScriere, &semnalFinal, sizeof(semnalFinal), &bytesScrise, NULL);
}

int main(int argumente, char* parametrii[]) {
    if (argumente == 3) {
        int limitaInferioara = atoi(parametrii[1]);
        int limitaSuperioara = atoi(parametrii[2]);
        HANDLE canalScriere = GetStdHandle(STD_OUTPUT_HANDLE);
        proceseazaNumere(limitaInferioara, limitaSuperioara, canalScriere);
        return 0;
    }

    const int MAX_NUMERE = 10000;
    const int NUM_PROC = 10;
    const int INTERVAL = MAX_NUMERE / NUM_PROC;

    HANDLE canale[NUM_PROC][2];
    PROCESS_INFORMATION detaliiProc[NUM_PROC];
    STARTUPINFOA detaliiStart[NUM_PROC];
    vector<int> listaFinalaPrime;

    for (int idx = 0; idx < NUM_PROC; ++idx) {
        SECURITY_ATTRIBUTES setariSecuritate;
        setariSecuritate.nLength = sizeof(SECURITY_ATTRIBUTES);
        setariSecuritate.lpSecurityDescriptor = NULL;
        setariSecuritate.bInheritHandle = TRUE;

        if (!CreatePipe(&canale[idx][0], &canale[idx][1], &setariSecuritate, 0)) {
            cerr << "Eroare la configurarea canalului de comunicare!" << endl;
            return 1;
        }

        ZeroMemory(&detaliiStart[idx], sizeof(detaliiStart[idx]));
        detaliiStart[idx].cb = sizeof(detaliiStart[idx]);
        detaliiStart[idx].hStdOutput = canale[idx][1];
        detaliiStart[idx].dwFlags |= STARTF_USESTDHANDLES;

        int limitaInferioara = idx * INTERVAL + 1;
        int limitaSuperioara = limitaInferioara + INTERVAL - 1;

        stringstream comandaProces;
        comandaProces << "\"" << parametrii[0] << "\" " << limitaInferioara << " " << limitaSuperioara;

        ZeroMemory(&detaliiProc[idx], sizeof(detaliiProc[idx]));
        if (!CreateProcessA(NULL, const_cast<LPSTR>(comandaProces.str().c_str()),
            NULL, NULL, TRUE, 0, NULL, NULL, &detaliiStart[idx], &detaliiProc[idx])) {
            cerr << "Eroare la lansarea procesului!" << endl;
            return 1;
        }

        CloseHandle(canale[idx][1]);
    }

    cout << "Rezultatele obtinute de fiecare proces:\n";
    for (int idx = 0; idx < NUM_PROC; ++idx) {
        DWORD bytesCititi;
        int valoarePrim;
        bool areRezultate = false;

        cout << "Procesul #" << idx + 1 << ": ";
        while (true) {
            if (ReadFile(canale[idx][0], &valoarePrim, sizeof(valoarePrim), &bytesCititi, NULL) && bytesCititi > 0) {
                if (valoarePrim == -1) break;
                cout << valoarePrim << " ";
                listaFinalaPrime.push_back(valoarePrim);
                areRezultate = true;
            }
            else {
                break;
            }
        }

        if (!areRezultate) {
            cout << "niciun numar prim.";
        }

        cout << endl;
        CloseHandle(canale[idx][0]);

        WaitForSingleObject(detaliiProc[idx].hProcess, INFINITE);
        CloseHandle(detaliiProc[idx].hProcess);
        CloseHandle(detaliiProc[idx].hThread);
    }

    cout << "\nLista completa de numere prime:\n";
    for (int prim : listaFinalaPrime) {
        cout << prim << " ";
    }
    cout << endl;

    return 0;
}
