#include <iostream>
#include <windows.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <format>

#define FORMAT_TIMESTAMP_SIZE 21
#define FORMAT_STATUS_SIZE 15
#define FORMAT_CALIBRATION_SIZE 21


enum Enum_MsgTypes{
    Calibration,
    Status,
    TimeStamp,
    NonFormat
};

// === Global Variables ===
std::string Global_PortName = "";
int Global_TimeStampCounter = 0;
int Global_TimestampFormatingFail = 0;

int Global_StatusFormatingFail = 0;
double Global_Status_Last[7] = {0};
double Global_Calibration[7] = {0};
double Global_MicrocontrollerTime = 0;
int Global_UpdateCounter = 0;


// === Functions ===
void updateConsole()
{
    system("cls"); // Clears the console
    // printk("#$#Status|$Stk.Free.Proc|%d|$Stk.Free.Sens|%d|$TmpComp.Sen.Avg|%lld|$TmpComp.Sen.Max|%lld|$TmpComp.Proc.T.Max|%lld|$TmpComp.Proc.IntPri.Avg|%lld|$TmpComp.Proc.Protec.Avg|%lld|\n",
    std::cout << "Versao 1.3 - " << __DATE__ << " - " << __TIME__ << std::endl;
    std::cout << "Update - " << Global_UpdateCounter << std::endl;
    std::cout << "Quantidade de TimeStamps Processadas ate o Momento: " << Global_TimeStampCounter << std::endl;
    std::cout << "Tempo de execucao do Microcontrolador: " << Global_MicrocontrollerTime << "ms" << std::endl;
    std::cout << "Status: " << std::endl;
    std::cout << "  Stack:" <<std::endl;
    std::cout << "      Thread Processador - Memoria Livre (Bytes): " << Global_Status_Last[0] << std::endl;
    std::cout << "      Thread Sensor - Memoria Livre (Bytes): " << Global_Status_Last[1] << std::endl;
    std::cout << "  Tempo de Execucao(us):" <<std::endl;
    std::cout << "      Sensor:" <<std::endl;
    std::cout << "          Media: " << Global_Status_Last[2] << std::endl;
    std::cout << "          Maximo: " << Global_Status_Last[3] << std::endl;
    std::cout << "      Processamento:" <<std::endl;
    std::cout << "          Media - Integral e Print: " << Global_Status_Last[5] << std::endl;
    std::cout << "          Media - Parte protegida: " << Global_Status_Last[6] << std::endl;
    std::cout << "          Media - Soma das Medias: " << (Global_Status_Last[5] + Global_Status_Last[6]) << std::endl;
    std::cout << "          Maximo: " << Global_Status_Last[4] << std::endl <<std::endl;
    
    std::cout << "Calibragem: " << std::endl;
    std::cout << "  Quantidade de Amostras: " << Global_Calibration[0] << std::endl;
    std::cout << "  X:" <<std::endl;
    std::cout << "      MinX: " << Global_Calibration[1] << std::endl;
    std::cout << "      MaxX: " << Global_Calibration[4] << std::endl;
    std::cout << "  Y:" <<std::endl;
    std::cout << "      MinY: " << Global_Calibration[2] << std::endl;
    std::cout << "      MaxY: " << Global_Calibration[5] << std::endl;
    std::cout << "  Z:" <<std::endl;
    std::cout << "      MinZ: " << Global_Calibration[3] << std::endl;
    std::cout << "      MaxZ: " << Global_Calibration[6] << std::endl;
    Global_UpdateCounter++;
}

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

void parseAndWriteCsvLine(const std::string& line, std::ofstream& outFile) {
    static bool firstline = true;
    std::stringstream ss(line);
    std::string segment;
    std::vector<std::string> segments;

    while (std::getline(ss, segment, '|')) {
        // Trim leading/trailing whitespace from each segment for clean CSV data
        size_t first = segment.find_first_not_of(" \n\r\t");
        if (std::string::npos == first) {
            segment = "";
        } else {
            size_t last = segment.find_last_not_of(" \n\r\t");
            segment = segment.substr(first, (last - first + 1));
        }
        segments.push_back(segment);
    }
    
    //Filter and process the segments
    if (segments[0] == "#$#Timestamp")
    {
        //printk("#$#Timestamp|$TmpAbs|%lld|$AcX|%lld|$AcY|%lld|$AcZ|%lld|$VelX|%lld|$VelY|%lld|$VelZ|%lld|$PosX|%lld|$PosY|%lld|$PosZ|%lld|\n",
        if (segments.size() != FORMAT_TIMESTAMP_SIZE)
        {
            Global_TimestampFormatingFail++;
            return;
        }

        //Process Data
        std::vector<std::string> ProcessedSegments;
        if (firstline)
        {
            for (size_t i = 1; i < segments.size(); ++i){
                //Check if the string is not empty and its first character is '$', indicating that it is a title cell
                if (!segments[i].empty() && segments[i][0] == '$') {
                    ProcessedSegments.push_back(segments[i].substr(1)); // Push the substring, starting from the second character to remove the '$'
                }
            }
            firstline = false;
        }
        else
        {
            for (size_t i = 1; i < segments.size(); ++i){
                //Check if the string is not empty and its first character is '$', indicating that it is a title cell
                if (!segments[i].empty() && segments[i][0] != '$') {
                    ProcessedSegments.push_back(segments[i]);
                }
            }

            //Get the Abs Time
            Global_MicrocontrollerTime = std::stod(ProcessedSegments[0]);

            //Apply the adjustment values. There should be a better way, but it works.
            ProcessedSegments[1] = std::format("{:.2f}", (std::stod(ProcessedSegments[1])/1000));
            ProcessedSegments[2] = std::format("{:.3f}", (std::stod(ProcessedSegments[2])/1000));
            ProcessedSegments[3] = std::format("{:.3f}", (std::stod(ProcessedSegments[3])/1000));
            ProcessedSegments[4] = std::format("{:.3f}", (std::stod(ProcessedSegments[4])/1000000));
            ProcessedSegments[5] = std::format("{:.3f}", (std::stod(ProcessedSegments[5])/1000000));
            ProcessedSegments[6] = std::format("{:.3f}", (std::stod(ProcessedSegments[6])/1000000));
            ProcessedSegments[7] = std::format("{:.3f}", (std::stod(ProcessedSegments[7])/1000000000));
            ProcessedSegments[8] = std::format("{:.3f}", (std::stod(ProcessedSegments[8])/1000000000));
            ProcessedSegments[9] = std::format("{:.3f}", (std::stod(ProcessedSegments[9])/1000000000));
        }

        Global_TimeStampCounter++;

        //Write on the CSV file
        for (size_t i = 0; i < ProcessedSegments.size(); ++i) {
        outFile << ProcessedSegments[i];
        // If it's not the last element, add a comma
        if (i < ProcessedSegments.size() - 1) {
            outFile << ",";
        }
        }
        outFile << std::endl;

    }
    else if(segments[0] == "#$#Status")
    {
        // printk("#$#Status|$Stk.Free.Proc|%d|$Stk.Free.Sens|%d|$TmpComp.Sen.Avg|%lld|$TmpComp.Sen.Max|%lld|$TmpComp.Proc.T.Max|%lld|$TmpComp.Proc.IntPri.Avg|%lld|$TmpComp.Proc.Protec.Avg|%lld|\n",
        if (segments.size() != FORMAT_STATUS_SIZE)
        {
            Global_StatusFormatingFail++;
            return;
        }

        //Process Data
        int counter = 0;
        for (size_t i = 1; i < segments.size(); ++i){
            //Check if the string is not empty and its first character is '$', that indicate a title and should be ignored
            if (!segments[i].empty() && segments[i][0] != '$') {
                Global_Status_Last[counter] = std::stod(segments[i]);
                counter ++;
            }
        }        

        //Print Status e da update no console
        updateConsole();
        return;
    }
    else if(segments[0] == "#$#Calibration"){
        //printk("#$#Calibration|$Smpl.Qtd|%d|$MinX|%d|$MinY|%d|$MinZ|%d|$MaxX|%d|$MaxY|%d|$MaxZ|%d|$AvgX|%d|$AvgY|%d|$AvgZ|%d|\n",
        
        if (segments.size() != FORMAT_CALIBRATION_SIZE)
        {
            std::cout << "Calibration Fail" << std::endl;
            return;
        }

        //Process Data
        int counter = 0;
        for (size_t i = 2; i < segments.size(); ++i){
            //Check if the string is not empty and its first character is '$', that indicate a title and should be ignored
            if (!segments[i].empty() && segments[i][0] != '$') {
                Global_Calibration[counter] = std::stod(segments[i]);
                counter++;
            }
        }        

        //Print Status e da update no console
        updateConsole();
        return;
    }
    else if(segments[0] == "$Start"){
        Global_UpdateCounter = 0;
        Global_MicrocontrollerTime = 0;
        Global_TimeStampCounter = 0;
        Global_TimestampFormatingFail = 0;
        Global_StatusFormatingFail = 0;
        updateConsole();
    }
    else{
        for (size_t i = 0; i < segments.size(); ++i) {
            std::cout << segments[i] << std::endl;
        }
        return;
    }
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

    Global_PortName = portName;
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
        std::cin.ignore(); //Pauses the console before closing it
        return 1;
    }

    std::cout << "Successfully connected to " << portName << std::endl;

    // 2. Configure the serial port settings
    DCB dcbSerialParams;
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        printError("getting comm state");
        CloseHandle(hSerial);
        std::cin.ignore(); //Pauses the console before closing it
        return 1;
    }

    dcbSerialParams.BaudRate = CBR_115200; // Baud Rate
    dcbSerialParams.ByteSize = 8;          // Data Bits
    dcbSerialParams.StopBits = ONESTOPBIT; // Stop Bits
    dcbSerialParams.Parity   = NOPARITY;   // Parity

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        printError("setting comm state");
        CloseHandle(hSerial);
        std::cin.ignore(); //Pauses the console before closing it
        return 1;
    }

    // 3. Set communication timeouts
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout         = 5000; // Max time between bytes
    timeouts.ReadTotalTimeoutConstant    = 5000; // Constant for total read time
    timeouts.ReadTotalTimeoutMultiplier  = 1000; // Multiplier for total read time
    timeouts.WriteTotalTimeoutConstant   = 5000;
    timeouts.WriteTotalTimeoutMultiplier = 1000;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        printError("setting timeouts");
        CloseHandle(hSerial);
        std::cin.ignore(); //Pauses the console before closing it
        return 1;
    }

    std::cout << "Waiting for data..." << std::endl;

    // 4. Read data from the serial port in a loop
    const int BUFFER_SIZE = 256;
    char readBuf[BUFFER_SIZE];
    DWORD bytesRead;
    std::string dataBuffer;
    std::ofstream csvFile;

    csvFile.open("output.csv");
    if (!csvFile.is_open()) {
        std::cerr << "Error: Could not open output.csv for writing." << std::endl;
        CloseHandle(hSerial);
        std::cin.ignore(); //Pauses the console before closing it
        return 1;
    }

    std::cout << "Saving data to output.csv..." << std::endl;

    while (true) {
        if (ReadFile(hSerial, readBuf, BUFFER_SIZE - 1, &bytesRead, NULL)) {
            if (bytesRead > 0) {
                readBuf[bytesRead] = '\0'; // Null-terminate for safety
                dataBuffer.append(readBuf, bytesRead);

                size_t pos = 0;
                // Process all complete lines in the buffer
                while ((pos = dataBuffer.find('\n')) != std::string::npos) {
                    std::string line = dataBuffer.substr(0, pos);
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    parseAndWriteCsvLine(line, csvFile);
                    dataBuffer.erase(0, pos + 1);
                }
            }
        } else {
            printError("reading from serial port");
            break; // Exit loop on error
        }
    }

    // 5. Clean up
    csvFile.close();
    CloseHandle(hSerial);
    std::cout << "\nSerial port closed." << std::endl;

    std::cin.ignore(); //Pauses the console before closing it
    return 0;
}
