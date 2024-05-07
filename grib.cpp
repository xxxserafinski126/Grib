#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

class GribDecoder {
public:
    explicit GribDecoder(const std::string& filename) {
        gribFile.open(filename, std::ios::binary | std::ios::in);
        if (!gribFile.is_open()) {
            std::cerr << "Failed to open the file!\n";
        }
    }

    ~GribDecoder() {
        if (gribFile.is_open()) {
            gribFile.close();
        }
    }

    void decodeAndDisplay() {
        if (!gribFile.is_open()) {
            std::cerr << "File is not open, cannot proceed with decoding.\n";
            return;
        }

        findGrib();
        find7777();
        if (startPosition != 0 && endPosition != 0) {
            displayMessageInfo();
            readSection1();
        }
        else {
            std::cerr << "Could not find complete GRIB message markers!\n";
        }
    }

private:
    std::fstream gribFile;
    uint32_t startPosition = 0;
    uint32_t endPosition = 0;
    uint32_t messageLength = 0;
    uint8_t editionNumber = 0;

    void findGrib() {
        char byte;
        while (gribFile.read(&byte, 1)) {
            if (byte == 'G') {
                char nextBytes[7];
                if (gribFile.read(nextBytes, 7) && strncmp(nextBytes, "RIB", 3) == 0) {
                    startPosition = gribFile.tellg();
                    startPosition -= 8; 
                   
                    messageLength = (static_cast<uint8_t>(nextBytes[3]) << 16) |
                        (static_cast<uint8_t>(nextBytes[4]) << 8) |
                        static_cast<uint8_t>(nextBytes[5]);
                    editionNumber = static_cast<uint8_t>(nextBytes[6]);
                    return;
                }
            }
        }
    }

    void find7777() {
        char byte;
        gribFile.seekg(0, std::ios::beg); 
        while (gribFile.read(&byte, 1)) {
            if (byte == '7') {
                char nextBytes[3];
                if (gribFile.read(nextBytes, 3) && strncmp(nextBytes, "777", 3) == 0) {
                    endPosition = gribFile.tellg();
                    endPosition -= 4; 
                    return;
                }
            }
        }
    }

    void displayMessageInfo() {
        std::cout << "\n==========================\n";
        std::cout << "== General Message Info ==\n";
        std::cout << "==========================\n";
        std::cout << "Grib found at position: " << startPosition << std::endl;
        std::cout << "7777 found at position: " << endPosition << std::endl;
        std::cout << "Message length from 'GRIB': " << messageLength << std::endl;
        std::cout << "Edition Number: " << static_cast<int>(editionNumber) << std::endl;
    }

    void readSection1() {
        std::cout << "\n======================\n";
        std::cout << "==  Section 1 Data  ==\n";
        std::cout << "======================\n";
        gribFile.seekg(startPosition + 8); 


        char lengthBytes[3];
        gribFile.read(lengthBytes, 3);
        uint32_t sectionLength = (static_cast<uint32_t>(lengthBytes[0] & 0xFF) << 16) |
            (static_cast<uint32_t>(lengthBytes[1] & 0xFF) << 8) |
            (static_cast<uint32_t>(lengthBytes[2] & 0xFF));


        std::vector<char> section1(sectionLength);
        gribFile.read(section1.data(), sectionLength);

        int tableVersion = section1[3];
        int identificationOfCentre = section1[4];
        int generationNumber = section1[5];
        int gridIdentification = section1[6];
        int flagGDSorBMS = section1[7];
        int parameterIndicator = section1[8];
        int typeOfLevel = section1[9];
        int level = (static_cast<int>(section1[10]) << 8) | static_cast<int>(section1[11]);

        std::cout << "Section 1 length: " << sectionLength << "\n";
        std::cout << "Parameter table Version: " << tableVersion << "\n";
        std::cout << "Identification of Centre: " << getCentreName(identificationOfCentre) << "\n";
        std::cout << "Generation number (model): " << generationNumber << "\n";
        std::cout << "Grid Identification: " << gridIdentification << "\n";
        std::cout << "GDS and BMS: " << ((flagGDSorBMS & 128) ? "GDS Included" : "GDS Omitted") << " " << ((flagGDSorBMS & 64) ? "BMS Included" : "BMS Omitted") << "\n";
        std::cout << "Parameter and units indicator: " << parameterIndicator << "\n";
        std::cout << "Type of level or layer: " << typeOfLevel << "\n";
        std::cout << "Level: " << level << "\n";
    }

    std::string getCentreName(int id) {
        switch (id) {
        case 7: return "US National Weather Service - NCEP (WMC)";
        case 98: return "ECMWF (European Centre for Medium-Range Weather Forecasts)";
        default: return "Unknown Centre";
        }
    }
};

int main() {
    GribDecoder decoder("all.grib");
    decoder.decodeAndDisplay();
    return 0;
}