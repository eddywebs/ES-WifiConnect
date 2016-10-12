#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include <cstring>
#include <vector>

// Struct: Wifi Object
struct WifiObject {
	int location;
	char name[1023];
	int endlocation;
};

// variables
const char* WIFIFILE = "/etc/wpa_supplicant/wpa_supplicant.conf";
std::string password;
std::string ssid;

// methods
void listNetworks(std::vector<WifiObject>* wifiNetworks);
void removeNetwork(WifiObject selectedNetwork, std::vector<WifiObject> networkList);
void deleteNetwork(WifiObject network);
void formatChar(char c[]);


int main(int argc, const char *argv[]){
	// Get ssid and password from arguments
	for (int i = 1; i < argc; i++){
		if(strcmp(argv[i], "--ssid") == 0){
			printf(argv[i + 1]);
			ssid = argv[i + 1];
		}

		if(strcmp(argv[i], "--password") == 0){
			password = argv[i + 1];
		}

		if (strcmp(argv[i], "--list") == 0) {
			std::vector<WifiObject> WifiNetworks;
			listNetworks(&WifiNetworks);

			// Read out vector.
			for (std::vector<WifiObject>::iterator net = WifiNetworks.begin(); net != WifiNetworks.end(); net++) {
				std::cout << (*net).name << "\n";
			}
			return 0;
		}

		if (strcmp(argv[i], "--remove") == 0) {
			if (argc < i) {
				std::cout << "1There was no network name given to remove.  Please do --remove [network name] \n";
				break;
			}

			WifiObject WifiNetwork;
			std::strcpy(WifiNetwork.name, argv[i + 1]);

			std::vector<WifiObject> Nodes;
			listNetworks(&Nodes);
			removeNetwork(WifiNetwork, Nodes);

		}

	}

	// check and make sure ssid is valid
	if (ssid.length() < 1) return -1;

	// open wpa_supplicant
	std::ofstream wpaFile;
	wpaFile.open(WIFIFILE, std::ofstream::app);

	if (wpaFile.is_open()){
		// Write new ssid and psk to file
		wpaFile << "\n";
		wpaFile << "# WIFIGUI SETUP \n";
		wpaFile << "network={\n";
		wpaFile << "     ssid=\"" << ssid << "\"\n";
		wpaFile << "     psk=\"" << password << "\"\n";
		wpaFile << "}\n";
		wpaFile << "# END SETUP";

		wpaFile.close();

		// Force wlan0 reset
		system("sudo ifup --force wlan0");

		std::cout << "0Connected to " << ssid << ".";

		return 1;
	}
	else {
		return -2;
	}

}


void listNetworks(std::vector<WifiObject>* wifiNetworks) {
	std::ifstream wpaFile;
	wpaFile.open(WIFIFILE);

	char line[1023];

	if (wpaFile.is_open()) {

		while (wpaFile.good()) {
			// Get location
			int loc = wpaFile.tellg();

			wpaFile.getline(line, 1023);
			if (strcmp(line, "# WIFIGUI SETUP ") == 0) {
				// Found a wifigui connected network, check if it's still there.
				wpaFile.getline(line, 1023);

				if (strcmp(line, "network={") == 0) {

					// Network is there, get name.
					wpaFile.getline(line, 1023);
					formatChar(line);
					WifiObject obj;
					obj.location = loc;
					strcpy(obj.name, line);

					// Skip password
					wpaFile.getline(line, 1023);

					// skip }
					wpaFile.getline(line, 1023);

					// get location of # END SETUP
					obj.endlocation = wpaFile.tellg();
					wpaFile.getline(line, 1023);

					wifiNetworks->push_back(obj);
				}
			}
		}

	}
}

void removeNetwork(WifiObject selectedNetwork, std::vector<WifiObject> networkList) {
	for (std::vector<WifiObject>::iterator net = networkList.begin(); net != networkList.end(); net++) {
		if (strcmp(selectedNetwork.name, (*net).name) == 0) {
			selectedNetwork.location = (*net).location;
			selectedNetwork.endlocation = (*net).endlocation;
			deleteNetwork(selectedNetwork);
		}
	}
}

void deleteNetwork(WifiObject network) {
	std::ifstream wpaFile;
	wpaFile.open(WIFIFILE);
	char strFile[1000][1023];
	char removedLines[1000][1023];

	char line[1023];
	int i = 0;

	// Read in file and skip over the network that is to be deleted.
	if (wpaFile.is_open()) {

		int r = 0;
		while (wpaFile.good() && i < 1001) {
			if (wpaFile.tellg() >= network.location && wpaFile.tellg() <= network.endlocation) {
				wpaFile.getline(removedLines[r], 1023);
				r++;
				continue;
			}
			wpaFile.getline(strFile[i], 1023);
			i++;
		}

	}

	// Re-write the file without the omited network
	std::ofstream oFile(WIFIFILE, std::ofstream::trunc);
	
	// If wpa_supplicant.conf can be opened
	if (oFile.is_open()) {
		for (int x = 0; x < i; x++) {
			oFile << strFile[x] << "\n";
		}
		oFile.close();

		// Force wlan0 reset
		system("sudo ifup --force wlan0");

		std::cout << "0The network: " << network.name << " has been removed from saved networks.";
	}
	else {
		// If the file is missing, or invalid permissions
		std::cout << "1Could not open wpa_supplicant.conf to write changes.\n";
	}
}


void formatChar(char c[]) {
	// Create a char array for holding a copy.  init it with null
	char newC[1023];
	for (int i = 0; i < 1023; i++) newC[i] = '\0';
	int x = 0;

	// Remove whitespace
	bool noMoreIndent = false;
	for (int i = 0; i < strlen(c); i++) {
		if (c[i] == ' ' && !noMoreIndent) continue;
		if (c[i] != ' ') noMoreIndent = true;
		newC[x] = c[i];
		x++;
	}

	// Remove ssid="
	std::string strC(newC);
	strC = strC.substr(6);

	// Remove ending "
	strC = strC.substr(0, strC.length() - 1);

	// Copy back to original c (char array)
	char *cc = &strC[0u];
	std::strcpy(c, cc);
}
