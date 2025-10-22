#include <iostream>
#include <windows.h>
#include <string>

void printError(const char* action) {
    DWORD error = GetLastError();
    char errorMsg[256];
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        errorMsg,
        sizeof(errorMsg),
        NULL
    );
    std::cerr << "Error " << action << ": " << error << " - " << errorMsg << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "\n\nVersao: " << __DATE__ << " - " << __TIME__ << std::endl;
    const char* portName;

    if (argc > 1) {
        portName = argv[1];
    } else {
        portName = "\\\\.\\COM3"; // Correctly escaped port name
        std::cout << "Usage: " << argv[0] << " <\\\\.\\\\COMx>" << std::endl;
        std::cout << "Defaulting to " << portName << std::endl << std::endl;
    }

    HANDLE hSerial;

    // 1. Open the serial port
    hSerial = CreateFileA(
        portName,
        GENERIC_READ, // We want to read from the port
        0,            // Exclusive access
        NULL,         // Default security attributes
        OPEN_EXISTING,// Open existing port
        0,            // Non-overlapped I/O
        NULL          // Null for comm devices
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        printError("opening serial port");
        return 1;
    }

    std::cout << "Successfully connected to " << portName << std::endl;

    // 2. Configure the serial port settings
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        printError("getting comm state");
        CloseHandle(hSerial);
        return 1;
    }

    dcbSerialParams.BaudRate = CBR_115200; // Baud Rate
    dcbSerialParams.ByteSize = 8;          // Data Bits
    dcbSerialParams.StopBits = ONESTOPBIT; // Stop Bits
    dcbSerialParams.Parity   = NOPARITY;   // Parity

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        printError("setting comm state");
        CloseHandle(hSerial);
        return 1;
    }

    // 3. Set communication timeouts
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout         = 50; // Max time between bytes
    timeouts.ReadTotalTimeoutConstant    = 50; // Constant for total read time
    timeouts.ReadTotalTimeoutMultiplier  = 10; // Multiplier for total read time
    timeouts.WriteTotalTimeoutConstant   = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        printError("setting timeouts");
        CloseHandle(hSerial);
        return 1;
    }

    std::cout << "Waiting for data..." << std::endl;

    // 4. Read data from the serial port in a loop
    const int BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE];
    DWORD bytesRead;

    while (true) {
        if (ReadFile(hSerial, buffer, BUFFER_SIZE - 1, &bytesRead, NULL)) {
            if (bytesRead > 0) {
                // Null-terminate the string
                buffer[bytesRead] = '\0';
                std::cout << buffer;
            }
        } else {
            printError("reading from serial port");
            break; // Exit loop on error
        }
    }

    // 5. Clean up
    CloseHandle(hSerial);
    std::cout << "\nSerial port closed." << std::endl;

    return 0;
}
