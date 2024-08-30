#define WIN32_LEAN_AND_MEAN
#include <windows.h>


#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define WINDOWS_INCLUDE
#include "../../../../../decompile/General/AltMods/OnlineCTR/global.h"
#include <enet/enet.h>

char *pBuf;
struct OnlineCTR* octr;

int buttonPrev[8] = { 0 };
char name[100];

ENetHost* clientHost;
ENetPeer* serverPeer;

struct Gamepad
{
	short unk_0;
	short unk_1;
	short stickLX;
	short stickLY;
	short stickLX_dontUse1;
	short stickLY_dontUse1;
	short stickRX;
	short stickRY;
	int buttonsHeldCurrFrame;
	int buttonsTapped;
	int buttonsReleased;
	int buttonsHeldPrevFrame;
};

void sendToHostUnreliable(const void* data, size_t size) {
	ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_UNSEQUENCED);
	enet_peer_send(serverPeer, 0, packet); // To do: have a look at the channels, maybe we want to use them better to categorize messages
}

void sendToHostReliable(const void* data, size_t size) {
	ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(serverPeer, 0, packet); // To do: have a look at the channels, maybe we want to use them better to categorize messages
}

void ProcessReceiveEvent(ENetPacket* packet)
{
	struct SG_Header* recvBuf = packet->data;
	int slot;

	//printf("received packet with type %i\n",((struct SG_Header*)recvBuf)->type);
	// switch will compile into a jmp table, no funcPtrs needed
	switch (((struct SG_Header*)recvBuf)->type)
	{
		case SG_ROOMS:
		{
			struct SG_MessageRooms* r = recvBuf;

			octr->ver_pc = VERSION;
			octr->ver_server = r->version;

			if (r->version != VERSION)
			{
				octr->CurrState = LAUNCH_ERROR;
				return;
			}

			if (octr->ver_psx != VERSION)
			{
				octr->CurrState = LAUNCH_ERROR;
				return;
			}

			// reopen the room menu,
			// either first time getting rooms,
			// or refresh after joining refused
			octr->serverLockIn2 = 0;

			octr->numRooms = r->numRooms;

			octr->clientCount[0x0] = r->numClients01;
			octr->clientCount[0x1] = r->numClients02;
			octr->clientCount[0x2] = r->numClients03;
			octr->clientCount[0x3] = r->numClients04;
			octr->clientCount[0x4] = r->numClients05;
			octr->clientCount[0x5] = r->numClients06;
			octr->clientCount[0x6] = r->numClients07;
			octr->clientCount[0x7] = r->numClients08;
			octr->clientCount[0x8] = r->numClients09;
			octr->clientCount[0x9] = r->numClients10;
			octr->clientCount[0xa] = r->numClients11;
			octr->clientCount[0xb] = r->numClients12;
			octr->clientCount[0xc] = r->numClients13;
			octr->clientCount[0xd] = r->numClients14;
			octr->clientCount[0xe] = r->numClients15;
			octr->clientCount[0xf] = r->numClients16;

			break;
		}

		// Assigned to room
		case SG_NEWCLIENT:
		{
			struct SG_MessageClientStatus* r = recvBuf;

			octr->DriverID = r->clientID;
			octr->NumDrivers = r->numClientsTotal;

			// default, disable cheats
			*(int*)&pBuf[0x80096b28 & 0xffffff] &=
				~(0x100000 | 0x80000 | 0x400 | 0x400000 | 0x10000);

			// odd-numbered index == even-number room
			// Index 1, 3, 5 -> Room 2, 4, 6
			if (octr->serverRoom == 0 || octr->serverRoom == 1 || octr->serverRoom == 2 || octr->serverRoom == 3 || octr->serverRoom == 4 || octr->serverRoom == 5 || octr->serverRoom == 6 || octr->serverRoom == 7)
			{
				r->special = 0;
			}
			if (octr->serverRoom == 8 || octr->serverRoom == 9 || octr->serverRoom == 10 || octr->serverRoom == 11)
			{
				r->special = 2;
			}
			if (octr->serverRoom == 12 || octr->serverRoom == 13 || octr->serverRoom == 14 || octr->serverRoom == 15)
			{
				r->special = 3;
			}
			
			octr->special = r->special;

#if 1
			// need to print, or compiler optimization throws this all away
			printf("\nEvento Especial: %d\n", octr->special);

			// SuperTurboPads
			if (octr->special == 2) *(int*)&pBuf[(0x80096b28) & 0xffffff] = 0x100000;
			// Icy Tracks
			if (octr->special == 3) *(int*)&pBuf[(0x80096b28) & 0xffffff] = 0x80000;
			// MAX WUMPA
			if (octr->special == 4) *(int*)&pBuf[(0x80096b28) & 0xffffff] = 0x10000;
#endif

			// offset 0x8
			octr->boolLockedInLap = 0;
			octr->boolLockedInLevel = 0;
			octr->lapID = 0;
			octr->levelID = 0;

			octr->boolLockedInCharacter = 0;
			octr->numDriversEnded = 0;

			memset(&octr->boolLockedInCharacters[0], 0, sizeof(octr->boolLockedInCharacters));
			memset(&octr->nameBuffer[0], 0, sizeof(octr->nameBuffer));
			memset(&octr->raceStats[0], 0, sizeof(octr->raceStats));

			// reply to server with your name
			memcpy(&octr->nameBuffer[0], &name, NAME_LEN);

			struct CG_MessageName m = { 0 };
			m.type = CG_NAME;
			memcpy(&m.name[0], &name[0], sizeof(m.name));
			sendToHostReliable(&m, sizeof(struct CG_MessageName));

			// choose to get host menu or guest menu
			octr->CurrState = LOBBY_ASSIGN_ROLE;
			break;
		}

		case SG_NAME:
		{
			struct SG_MessageName* r = recvBuf;

			int clientID = r->clientID;
			if (clientID == octr->DriverID) break;
			if (clientID < octr->DriverID) slot = clientID + 1;
			if (clientID > octr->DriverID) slot = clientID;

			octr->NumDrivers = r->numClientsTotal;

			memcpy(&octr->nameBuffer[slot], &r->name[0], NAME_LEN);

			// handle disconnection
			if (r->name[0] == 0)
			{
				// make this player hold SQUARE
				struct Gamepad* pad = &pBuf[(0x80096804 + (slot * 0x50)) & 0xffffff];
				pad->buttonsHeldCurrFrame = 0x20;
				pad->buttonsTapped = 0;
				pad->buttonsReleased = 0;
				pad->buttonsHeldPrevFrame = 0x20;
			}

			break;
		}

		case SG_TRACK:
		{
			struct SG_MessageTrack* r = recvBuf;

			// 1,3,5,7
			int numLaps = (r->lapID * 2) + 1;

			if(r->lapID == 4) numLaps = 15;
			if(r->lapID == 5) numLaps = 30;
			if(r->lapID == 6) numLaps = 69;
			if(r->lapID == 7) numLaps = 120;

			// set sdata->gGT->numLaps
			*(char*)&pBuf[(0x80096b20 + 0x1d33) & 0xffffff] = numLaps;

			octr->levelID = r->trackID;
			octr->CurrState = LOBBY_CHARACTER_PICK;

			break;
		}

		case SG_CHARACTER:
		{
			struct SG_MessageCharacter* r = recvBuf;

			int clientID = r->clientID;
			int characterID = r->characterID;

			if (clientID == octr->DriverID) break;
			if (clientID < octr->DriverID) slot = clientID + 1;
			if (clientID > octr->DriverID) slot = clientID;

			*(short*)&pBuf[(0x80086e84 + 2 * slot) & 0xffffff] = characterID;
			octr->boolLockedInCharacters[clientID] = r->boolLockedIn;

			break;
		}

		case SG_STARTLOADING:
		{
			// variable reuse, wait a few frames,
			// so screen updates with green names
			octr->CountPressX = 0;
			octr->CurrState = LOBBY_START_LOADING;

			break;
		}

		case SG_STARTRACE:
		{
			octr->CurrState = GAME_START_RACE;

			break;
		}

		case SG_RACEDATA:
		{
			// wait for drivers to be initialized
			if (octr->CurrState < GAME_WAIT_FOR_RACE)
				break;

			int sdata_Loading_stage =
				*(int*)&pBuf[0x8008d0f8 & 0xffffff];

			if (sdata_Loading_stage != -1)
				break;

			struct SG_EverythingKart* r = recvBuf;

			int clientID = r->clientID;
			if (clientID == octr->DriverID) break;
			if (clientID < octr->DriverID) slot = clientID + 1;
			if (clientID > octr->DriverID) slot = clientID;

			int curr = r->buttonHold;

			// sneak L1/R1 into one byte,
			// remove Circle/L2

			if ((curr & 0x40) != 0)
			{
				curr &= ~(0x40);
				curr |= 0x400;
			}

			if ((curr & 0x80) != 0)
			{
				curr &= ~(0x80);
				curr |= 0x800;
			}

			int prev = buttonPrev[slot];

			// tapped
			int tap = ~prev & curr;

			// released
			int rel = prev & ~curr;

			struct Gamepad* pad = &pBuf[(0x80096804 + (slot * 0x50)) & 0xffffff];
			pad->buttonsHeldCurrFrame = curr;
			pad->buttonsTapped = tap;
			pad->buttonsReleased = rel;
			pad->buttonsHeldPrevFrame = prev;

			// In this order: Up, Down, Left, Right
			if ((pad->buttonsHeldCurrFrame & 1) != 0) pad->stickLY = 0;
			else if ((pad->buttonsHeldCurrFrame & 2) != 0) pad->stickLY = 0xFF;
			else pad->stickLY = 0x80;

			if ((pad->buttonsHeldCurrFrame & 4) != 0) pad->stickLX = 0;
			else if ((pad->buttonsHeldCurrFrame & 8) != 0) pad->stickLX = 0xFF;
			else pad->stickLX = 0x80;

			buttonPrev[slot] = curr;

			int psxPtr = *(int*)&pBuf[(0x8009900c + (slot * 4)) & 0xffffff];
			psxPtr &= 0xffffff;

			// lossless compression, bottom byte is never used,
			// cause psx renders with 3 bytes, and top byte
			// is never used due to world scale (just pure luck)
			*(int*)&pBuf[psxPtr + 0x2d4] = ((int)r->posX) * 256;
			*(int*)&pBuf[psxPtr + 0x2d8] = ((int)r->posY) * 256;
			*(int*)&pBuf[psxPtr + 0x2dc] = ((int)r->posZ) * 256;

			int angle =
				(r->kartRot1) |
				(r->kartRot2 << 5);

			angle &= 0xfff;

			*(short*)&pBuf[psxPtr + 0x39a] = (short)angle;

			// keep setting to 200,
			// and if !boolReserves, let it fall to zero
			if (r->boolReserves)
				*(short*)&pBuf[psxPtr + 0x3E2] = 200;

			*(short*)&pBuf[psxPtr + 0x30] = r->wumpa;

			break;
		}

		case SG_WEAPON:
		{
			struct SG_MessageWeapon* r = recvBuf;

			int clientID = r->clientID;
			if (clientID == octr->DriverID) break;
			if (clientID < octr->DriverID) slot = clientID + 1;
			if (clientID > octr->DriverID) slot = clientID;

			octr->Shoot[slot].boolNow = 1;
			octr->Shoot[slot].Weapon = r->weapon;
			octr->Shoot[slot].boolJuiced = r->juiced;
			octr->Shoot[slot].flags = r->flags;

			break;
		}

		case SG_ENDRACE:
		{
			struct SG_MessageEndRace* r = recvBuf;
			int clientID = r->clientID;
			if (clientID == octr->DriverID) break;
			if (clientID < octr->DriverID) slot = clientID + 1;
			if (clientID > octr->DriverID) slot = clientID;

			// make this player hold SQUARE
			struct Gamepad* pad = &pBuf[(0x80096804 + (slot * 0x50)) & 0xffffff];
			pad->buttonsHeldCurrFrame = 0x20;
			pad->buttonsTapped = 0;
			pad->buttonsReleased = 0;
			pad->buttonsHeldPrevFrame = 0x20;

			octr->raceStats[octr->numDriversEnded].slot = slot;
			memcpy(&octr->raceStats[octr->numDriversEnded].finalTime, &r->courseTime, sizeof(r->courseTime));
			memcpy(&octr->raceStats[octr->numDriversEnded].bestLap, &r->lapTime, sizeof(r->lapTime));
			octr->numDriversEnded++;

			break;
		}

	default:
		break;
	}

}

void ProcessNewMessages()
{
#define AUTO_RETRY_SECONDS 10
#define ESC_KEY 27 // ASCII value for ESC key

	ENetEvent event;
	char response = 0;

	while (enet_host_service(clientHost, &event, 0) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_RECEIVE:
			ProcessReceiveEvent(event.packet);
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			// command prompt reset
			system("cls");
			PrintBanner(SHOW_NAME);
			printf("\nClient: Connection Dropped (Server Full or Server Offline)...  ");

			// to go the lobby browser
			octr->CurrState = -1;
			break;

		default:
			break;
		}

		enet_packet_destroy(event.packet);
	}
}

void PrintBanner(char show_name)
{
    printf("\033[91m        ____ _____ ____    ___ _                     \n");
    printf("\033[91m  ___  \033[93m/ ___|_   _|  _ \\  |_ _| |_ ___ _ __ ___  ___ \n");
    printf("\033[92m / _ \\| |     | | | |_) |  | || __/ _ \\ '_ ` _ \\/ __|\n");
    printf("\033[96m| (_) | |___  | | |  _ <   | || ||  __/ | | | | \\__ \\\n");
    printf("\033[94m \\___/ \\____| |_| |_| \\_\\ |___|\\__\\___|_| |_| |_|___/\n\033[0m");
	printf(" Cliente de OnlineCTR (Sin soporte oficial)\n Build %s (%s)\n\n", __DATE__, __TIME__);

	if(show_name == true) printf(" Bienvenido a OnlineCTR: Gasmoxian %s!\n", name);
}

void StartAnimation()
{
	char spinner_chars[] = "|/-\\";
	static int spinner_length = sizeof(spinner_chars) - 1; // exclude the NULL terminator
	static int delay = 0;
	static int i = 0;

	printf("\b%c", spinner_chars[i]); // '\b' moves the cursor back one position
	fflush(stdout); // ensure the output is printed immediately

	i = (i + 1) % spinner_length;
}

void StopAnimation()
{
	printf("\b \b\n"); // clear the spinner character when done
	fflush(stdout); // ensure the output is printed immediately
}

void DisconSELECT()
{
	int hold = *(int*)&pBuf[(0x80096804 + 0x10) & 0xffffff];

	if((hold & 0x2000) != 0)
	{
		// Sleep() triggers server timeout
		// just in case client isnt disconnected
		StopAnimation();
		printf("Cliente: Desconectado (ID: DSELECT)...  ");
		enet_peer_disconnect_now(serverPeer, 0);
		serverPeer = 0;

		// to go the lobby browser
		octr->CurrState = -1;

		return;
	}
}

void ClearInputBuffer()
{
	int c;

	while ((c = getchar()) != '\n' && c != EOF);
}

void StatePC_Launch_EnterPID()
{
	// if client connected to DuckStation
	// before game booted, wait for boot
	if (!octr->IsBootedPS1)
		return;

	StopAnimation();
	printf("Cliente: Esperando conectarse a un servidor...  ");
	octr->CurrState = LAUNCH_PICK_SERVER;
}

void printUntilPeriod(const char *str)
{
	int i = 0;

	// loop through each character in the string
	while (str[i] != '\0')
	{
		// break the loop if a period is found
		if (str[i] == '.') break;

		// print the character
		putchar(toupper(str[i]));
		i++;
	}
}

int StaticServerID=0;
int StaticRoomID=0;
void StatePC_Launch_PickServer()
{
	ENetAddress addr;
	static unsigned char dns_string[32] = { 0 };
	static unsigned char localServer;

	// local server
	char ip[100];
	char portStr[PORT_SIZE];
	int port;

	// quit if disconnected, but not loaded
	// back into the selection screen yet
	int gGT_levelID =
		*(int*)&pBuf[(0x80096b20+0x1a10) & 0xffffff];

	// must be in cutscene level to see country selector
	if (gGT_levelID != 0x26)
		return;

	// quit if in loading screen (force-reconnect)
	int sdata_Loading_stage =
		*(int*)&pBuf[0x8008d0f8 & 0xffffff];
	if (sdata_Loading_stage != -1)
		return;

	if (serverPeer != 0)
	{
		enet_peer_disconnect_now(serverPeer, 0);
		serverPeer = 0;
	}

	// return now if the server selection hasn't been selected yet
	if (octr->serverLockIn1 == 0)
		return;

	// === Now Selecting Country ===
	octr->boolClientBusy = 1;

	StaticServerID = octr->serverCountry;

	switch (octr->serverCountry)
	{
		// EUROPE (Unknown Location)
		case 0:	
		{
			strcpy_s(dns_string, sizeof(dns_string), "0.0.0.0");
			enet_address_set_host(&addr, dns_string);
			addr.port = 64001;

			break;
		}

		// USA (New York City)
		case 1:
		{
			strcpy_s(dns_string, sizeof(dns_string), "0.0.0.0");
			enet_address_set_host(&addr, dns_string);
			addr.port = 1234;

			break;
		}

		// Mexico (USA West)
		case 2:
		{
			strcpy_s(dns_string, sizeof(dns_string), "mednafen-peru2.ddns.net");
			enet_address_set_host(&addr, dns_string);
			addr.port = 1234;

			break;
		}

		// BRAZIL (Unknown Location)
		case 3:
		{
			strcpy_s(dns_string, sizeof(dns_string), "0.0.0.0");
			enet_address_set_host(&addr, dns_string);
			addr.port = 64001;

			break;
		}

		// AUSTRALIA (Sydney)
		case 4:
		{
			strcpy_s(dns_string, sizeof(dns_string), "22.ip.gl.ply.gg");
			enet_address_set_host(&addr, dns_string);
			addr.port = 21776;

			break;
		}

		// SINGAPORE (Unknown Location)
		case 5:
		{
			strcpy_s(dns_string, sizeof(dns_string), "127.0.0.1");
			enet_address_set_host(&addr, dns_string);
			addr.port = 6969;

			break;
		}

		// BETA (New Jersey)
		case 6:
		{
			strcpy_s(dns_string, sizeof(dns_string), "usa1.online-ctr.net");
			enet_address_set_host(&addr, dns_string);
			addr.port = 64001;

			break;
		}
	
	    // PRIVATE SERVER
		case 7:
		{
        StopAnimation();

        // Ruta del archivo que contiene la IP y el puerto
        const char* filePath = ".\\data\\host\\host.txt";
        FILE* file;
        errno_t err = fopen_s(&file, filePath, "r");

        if (err != 0)
        {
            printf("\nError: Could not open file %s\n", filePath);
            break; // Salir si no se puede abrir el archivo
        }

        // Leer la IP desde el archivo
        if (fgets(ip, sizeof(ip), file) == NULL)
        {
            printf("\nError: Could not read IP from file!\n");
            fclose(file);
            break;
        }

        // Remover el carácter de nueva línea (si está presente)
        ip[strcspn(ip, "\n")] = '\0';

        // Leer el puerto desde el archivo
        if (fgets(portStr, sizeof(portStr), file) == NULL)
        {
            printf("\nError: Could not read port from file!\n");
            fclose(file);
            break;
        }

        // Remover el carácter de nueva línea (si está presente)
        portStr[strcspn(portStr, "\n")] = '\0';

        // Convertir el puerto de string a entero y validar el rango
        port = atoi(portStr);

        if (port < 0 || port > 65535)
        {
            printf("\nError: Port value out of range!\n");
            fclose(file);
            break;
        }

        // Cerrar el archivo después de leer
        fclose(file);

        // Configurar la IP y el puerto en la estructura addr
        enet_address_set_host(&addr, ip);
        addr.port = port;

        localServer = true;

        break;
		}
	}

	StopAnimation();
	printf("Cliente: Intentando conectar a \"");
	if(localServer == false) printUntilPeriod(dns_string);
	else printf("%s:%d", ip, addr.port);
	printf("\" (ID: %d)...  ", StaticServerID);

	clientHost = enet_host_create(NULL /* create a client host */,
		1 /* only allow 1 outgoing connection */,
		2 /* allow up 2 channels to be used, 0 and 1 */,
		0 /* assume any amount of incoming bandwidth */,
		0 /* assume any amount of outgoing bandwidth */);

	if (clientHost == NULL)
	{
		fprintf(stderr, "Error: Error al crear una conexion enet en el cliente!\n");
		exit(EXIT_FAILURE);
	}

	if (serverPeer != 0)
	{
		enet_peer_disconnect_now(serverPeer, 0);
		serverPeer = 0;
	}

	serverPeer = enet_host_connect(clientHost, &addr, 2, 0);

	if (serverPeer == NULL)
	{
		fprintf(stderr, "Error: No hay peers disponibles para crear una conexion enet!\n");
		exit(EXIT_FAILURE);
	}

	//fprintf(stderr, "Trying to establish connection with server at %s:%i\n", ip, adress.port);

	ENetEvent event;

	int retryCount = 0;
	char connected = false;
	#define MAX_RETRIES 3

	// retry loop to attempt a reconnection
	while (retryCount < MAX_RETRIES && !connected)
	{
		// wait up to 3 seconds for the connection attempt to succeed
		if (enet_host_service(clientHost, &event, 3000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
		{
			StopAnimation();
			printf("Cliente: Conectado con exito!  ");

			connected = true;
		}
		else
		{
			StopAnimation();
			printf("Error: No se pudo conectar al servidor, reintentando %d/%d...  ", retryCount + 1, MAX_RETRIES);

			if (retryCount >= MAX_RETRIES)
			{
				// to go the country select
				octr->CurrState = 1;
				octr->boolClientBusy = 0;
				return;
			}

			retryCount++;
		}
	}

	// 5 seconds
	enet_peer_timeout(serverPeer, 1000000, 1000000, 5000);

	octr->DriverID = -1;
	octr->CurrState = LAUNCH_PICK_ROOM;
	octr->boolClientBusy = 0;
}

void StatePC_Launch_Error()
{
	// do nothing
}

int connAttempt = 0;
int countFrame = 0;
void StatePC_Launch_PickRoom()
{
	ProcessNewMessages();

	countFrame++;
	if (countFrame == 60)
	{
		countFrame = 0;

		// send junk data,
		// this triggers server response
		struct CG_MessageRoom mr;
		mr.type = CG_JOINROOM;
		mr.room = 0xFF;
		sendToHostReliable(&mr, sizeof(struct CG_MessageRoom));
	}

	// wait for room to be chosen
	if (!octr->serverLockIn2)
	{
		connAttempt = 0;
		return;
	}

	// dont send CG_JoinRoom twice
	if (connAttempt == 1)
		return;

	connAttempt = 1;

	struct CG_MessageRoom mr;
	mr.type = CG_JOINROOM;
	mr.room = octr->serverRoom;
	sendToHostReliable(&mr, sizeof(struct CG_MessageRoom));
}

void StatePC_Lobby_AssignRole()
{
	connAttempt = 0;
	countFrame = 0;
	ProcessNewMessages();
}

void StatePC_Lobby_HostTrackPick()
{
	ProcessNewMessages();

	// boolLockedInLap gets set after
	// boolLockedInLevel already sets
	if (!octr->boolLockedInLap) return;

	StopAnimation();
	printf("Clienet: Enviando pista al servidor...  ");

	struct CG_MessageTrack mt = { 0 };
	mt.type = CG_TRACK;

	mt.trackID = octr->levelID;
	mt.lapID = octr->lapID;

	// 1,3,5,7
	int numLaps = (mt.lapID * 2) + 1;

	if(mt.lapID == 4) numLaps = 30;
	if(mt.lapID == 5) numLaps = 60;
	if(mt.lapID == 6) numLaps = 90;
	if(mt.lapID == 7) numLaps = 120;

	// sdata->gGT->numLaps
	*(char*)&pBuf[(0x80096b20 + 0x1d33) & 0xffffff] = numLaps;

	sendToHostReliable(&mt, sizeof(struct CG_MessageTrack));

	octr->CurrState = LOBBY_CHARACTER_PICK;
}

int prev_characterID = -1;
int prev_boolLockedIn = -1;

void StatePC_Lobby_GuestTrackWait()
{
	ProcessNewMessages();

	prev_characterID = -1;
	prev_boolLockedIn = -1;
}

void StatePC_Lobby_CharacterPick()
{
	ProcessNewMessages();

	struct CG_MessageCharacter mc = { 0 };
	mc.type = CG_CHARACTER;

	// data.characterIDs[0]
	mc.characterID = *(char*)&pBuf[0x80086e84 & 0xffffff];
	mc.boolLockedIn = octr->boolLockedInCharacters[octr->DriverID];

	if(
		(prev_characterID != mc.characterID) ||
		(prev_boolLockedIn != mc.boolLockedIn)
	  )
	{
		prev_characterID = mc.characterID;
		prev_boolLockedIn = mc.boolLockedIn;

		sendToHostReliable(&mc, sizeof(struct CG_MessageCharacter));
	}

	if (mc.boolLockedIn == 1) octr->CurrState = LOBBY_WAIT_FOR_LOADING;
}

void StatePC_Lobby_WaitForLoading()
{
	ProcessNewMessages();

	// if recv message to start loading,
	// change state to StartLoading,
	// this check happens in ProcessNewMessages
}

int boolAlreadySent_StartRace = 0;
int boolAlreadySent_EndRace = 0;

void StatePC_Lobby_StartLoading()
{
	ProcessNewMessages();

	boolAlreadySent_StartRace = 0;
	boolAlreadySent_EndRace = 0;
}

void SendEverything()
{
	struct CG_EverythingKart cg = { 0 };
	cg.type = CG_RACEDATA;

	// === Buttons ===
	int hold = *(int*)&pBuf[(0x80096804 + 0x10) & 0xffffff];

	// ignore Circle/L2
	hold &= ~(0xC0);

	// put L1/R1 into one byte
	if ((hold & 0x400) != 0) hold |= 0x40;
	if ((hold & 0x800) != 0) hold |= 0x80;

	cg.buttonHold = (unsigned char)hold;

	// === Position ===
	int psxPtr = *(int*)&pBuf[0x8009900c & 0xffffff];
	psxPtr &= 0xffffff;

	// lossless compression, bottom byte is never used,
	// cause psx renders with 3 bytes, and top byte
	// is never used due to world scale (just pure luck)
	cg.posX = (short)(*(int*)&pBuf[psxPtr + 0x2d4] / 256);
	cg.posY = (short)(*(int*)&pBuf[psxPtr + 0x2d8] / 256);
	cg.posZ = (short)(*(int*)&pBuf[psxPtr + 0x2dc] / 256);

	// === Direction Faced ===
	// driver->0x39a (direction facing)
	unsigned short angle = *(unsigned short*)&pBuf[psxPtr + 0x39a];
	angle &= 0xfff;

	unsigned char angleBit5 = angle & 0x1f;
	unsigned char angleTop8 = angle >> 5;
	cg.kartRot1 = angleBit5;
	cg.kartRot2 = angleTop8;

	char wumpa = *(unsigned char*)&pBuf[psxPtr + 0x30];
	cg.wumpa = wumpa;

	// must be read as unsigned, even though game uses signed,
	// has to do with infinite reserves when the number is negative
	unsigned short reserves = *(unsigned short*)&pBuf[psxPtr + 0x3E2];
	cg.boolReserves = (reserves > 200);

	// TO DO: No Fire Level yet

	sendToHostUnreliable(&cg, sizeof(struct CG_EverythingKart));

	if (octr->Shoot[0].boolNow == 1)
	{
		octr->Shoot[0].boolNow = 0;

		struct CG_MessageWeapon w = { 0 };

		w.type = CG_WEAPON;
		w.weapon = octr->Shoot[0].Weapon;
		w.juiced = octr->Shoot[0].boolJuiced;
		w.flags = octr->Shoot[0].flags;

		sendToHostReliable(&w, sizeof(struct CG_MessageWeapon));
	}
}

void StatePC_Game_WaitForRace()
{
	ProcessNewMessages();

	int gGT_gameMode1 = *(int*)&pBuf[(0x80096b20 + 0x0) & 0xffffff];

	if (
			// only send once
			(!boolAlreadySent_StartRace) &&

			// after camera fly-in is done
			((gGT_gameMode1 & 0x40) == 0)
		)
	{
		StopAnimation();
		printf("Cliente: La carrera esta a punto de iniciar...  ");
		boolAlreadySent_StartRace = 1;

		struct CG_Header cg = { 0 };
		cg.type = CG_STARTRACE;
		sendToHostReliable(&cg, sizeof(struct CG_Header));
	}

	SendEverything();
}

void StatePC_Game_StartRace()
{
	ProcessNewMessages();
	SendEverything();

	// not using this special event
	#if 0
	int gGT_levelID =
		*(int*)&pBuf[(0x80096b20 + 0x1a10) & 0xffffff];

	// Friday demo mode camera
	if(octr->special == 3)
		if(gGT_levelID < 18)
			*(short*)&pBuf[(0x80098028) & 0xffffff] = 0x20;
	#endif
}

#include <time.h>
clock_t timeStart;
void StatePC_Game_EndRace()
{
	ProcessNewMessages();

	if (!boolAlreadySent_EndRace)
	{
		boolAlreadySent_EndRace = 1;

		int psxPtr = *(int*)&pBuf[0x8009900c & 0xffffff];
		psxPtr &= 0xffffff;

		struct CG_MessageEndRace cg = { 0 };
		cg.type = CG_ENDRACE;

		memcpy(&cg.courseTime, &pBuf[psxPtr + DRIVER_COURSE_OFFSET], sizeof(cg.courseTime));
		memcpy(&cg.lapTime, &pBuf[psxPtr + DRIVER_BESTLAP_OFFSET], sizeof(cg.courseTime));
		sendToHostReliable(&cg, sizeof(struct CG_MessageEndRace));

		// end race for yourself
		octr->raceStats[octr->numDriversEnded].slot = 0;
		octr->raceStats[octr->numDriversEnded].finalTime = *(int*)&pBuf[psxPtr + DRIVER_COURSE_OFFSET];
		octr->raceStats[octr->numDriversEnded].bestLap = *(int*)&pBuf[psxPtr + DRIVER_BESTLAP_OFFSET];
		octr->numDriversEnded++;

		// if you finished last
		timeStart = clock();
	}

	int numDead = 0;

	for (int i = 0; i < octr->NumDrivers; i++)
	{
		if (octr->nameBuffer[i][0] == 0) numDead++;
	}
}

void (*ClientState[]) () = {
	StatePC_Launch_EnterPID,		// 0
	StatePC_Launch_PickServer,		// 1
	StatePC_Launch_PickRoom,		// 2
	StatePC_Launch_Error,			// 3
	StatePC_Lobby_AssignRole,		// 4
	StatePC_Lobby_HostTrackPick,	// 5
	StatePC_Lobby_GuestTrackWait,	// 6
	StatePC_Lobby_CharacterPick,	// 7
	StatePC_Lobby_WaitForLoading,	// 8
	StatePC_Lobby_StartLoading,		// 9
	StatePC_Game_WaitForRace,		// 10
	StatePC_Game_StartRace,			// 11
	StatePC_Game_EndRace			// 12
};

// for EnumProcessModules
#pragma comment(lib, "psapi.lib")

int main()
{
	HWND console = GetConsoleWindow();
	RECT r;
	GetWindowRect(console, &r); // stores the console's current dimensions
	MoveWindow(console, r.left, r.top, 800, 480 + 35, TRUE);
	SetConsoleOutputCP(CP_UTF8); // force the output to be unicode (UTF-8)

	PrintBanner(DONT_SHOW_NAME);

	// ask for the users online identification
	printf("Introduce tu nombre de usuario: ");
	scanf_s("%s", name, (int)sizeof(name));
	name[NAME_LEN - 1] = 0; // truncate the name

	// show a welcome message
	system("cls");
	PrintBanner(SHOW_NAME);
	printf("\n");

	int numDuckInstances = 0;
	char* duckTemplate = "duckstation";
	int duckPID = -1;

	// copy from
	// https://learn.microsoft.com/en-us/windows/win32/psapi/enumerating-all-processes
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded);
	cProcesses = cbNeeded / sizeof(DWORD);

	for (int i = 0; i < cProcesses; i++)
	{
		DWORD processID = aProcesses[i];

		if (processID != 0)
		{
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

			if (NULL != hProcess)
			{
				HMODULE hMod;
				DWORD cbNeeded;

				if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
				{
					TCHAR szProcessName[MAX_PATH];
					GetModuleBaseNameA(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));

					char* procName = (char*)&szProcessName[0];

					if (
						(*(int*)&procName[0] == *(int*)&duckTemplate[0]) &&
						(*(int*)&procName[4] == *(int*)&duckTemplate[4])
						)
					{
						numDuckInstances++;
						duckPID = processID;
					}
				}
			}
		}
	}

	if (numDuckInstances == 0)
	{
		printf("Error: No se esta ejecutando Duckstation!\n\n");
		system("pause");
		exit(0);
	}
	else printf("Cliente: Hemos detectado una instancia de duckstation\n");

	char pidStr[16];

	if (numDuckInstances > 1)
	{
		printf("Cuidado: Tienes muchos patos en el lagol\n");
		printf("Cierra todos y reintenta o bien, selecciona un PID\n\n");

		printf("Introduce el PID de tu pato: ");
		scanf_s("%s", pidStr, (int)sizeof(pidStr));
	}
	else
	{
		sprintf_s(pidStr, 100, "%d", duckPID);
	}

	char duckName[100];
	sprintf_s(duckName, 100, "duckstation_%s", pidStr);

	TCHAR duckNameT[100];
	swprintf(duckNameT, 100, L"%hs", duckName);

	// 8 MB RAM
	const unsigned int size = 0x800000;
	HANDLE hFile = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, duckNameT);
	pBuf = (char*)MapViewOfFile(hFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, size);

	if (pBuf == 0)
	{
		printf("Error: No se pudo abrir DuckStation!\n\n");
		system("pause");
		system("cls");
		main();
	}

	octr = (struct OnlineCTR*)&pBuf[0x8000C000 & 0xffffff];

	// initialize enet
	if (enet_initialize() != 0)
	{
		printf(stderr, "Error: No se pudo inicializar eNet!\n");

		return 1;
	}

	atexit(enet_deinitialize);
	printf("Cliente: Esperando al binario de oCTR Items...  ");

	while (1)
	{
		// To do: Check for PS1 system clock tick then run the client update
		octr->windowsClientSync++;

		// should rename to room selection
		if (octr->CurrState >= LAUNCH_PICK_ROOM)
			DisconSELECT();

		StartAnimation();

		if (octr->CurrState >= 0)
			ClientState[octr->CurrState]();

		void FrameStall(); FrameStall();
	}

	printf("\n");
	system("pause");
}

#ifdef __WINDOWS__
void usleep(__int64 usec)
{
	HANDLE timer;
	LARGE_INTEGER ft = { 0 };

	ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}
#endif

#pragma optimize("", off)
	int gGT_timer = 0;

	void FrameStall()
	{
		// wait for next frame
		while (gGT_timer == *(int*)&pBuf[(0x80096b20 + 0x1cf8) & 0xffffff])
		{
			usleep(1);
		}

		gGT_timer = *(int*)&pBuf[(0x80096b20 + 0x1cf8) & 0xffffff];
	}
#pragma optimize("", on)