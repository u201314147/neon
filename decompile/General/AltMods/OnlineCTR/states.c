#include <common.h>
#include "global.h"

extern struct RectMenu menu;

void StatePS1_Launch_EnterPID()
{
	DECOMP_DecalFont_DrawLine(
		"Ejecuta el cliente para continuar",
		0x100,0xA8,FONT_SMALL,JUSTIFY_CENTER|ORANGE);
}

extern char* countryNames[8];
bool initString = true;

void StatePS1_Launch_PickServer()
{
	if (initString)
	{
		strcpy(sdata->lngStrings[0x4e], "OnlineCTR");
		initString = false;
	}
	
	DecalFont_DrawLine("Bienvenido a ",0x10,0x10,FONT_SMALL,PENTA_WHITE);
	DecalFont_DrawLine("OnlineCTR Gasmoxian",0x10,0x18,FONT_SMALL,OXIDE_LIGHT_GREEN);
	DecalFont_DrawLine("Lista de cambios:",0x10,0x48,FONT_SMALL,PAPU_YELLOW);
	DecalFont_DrawLine("- Items rebalanceados",0x10,0x58,FONT_SMALL,PENTA_WHITE);
	DecalFont_DrawLine("- Vueltas extras reactivadas",0x10,0x60,FONT_SMALL,PENTA_WHITE);
	DecalFont_DrawLine("- Eventos reactivados:",0x10,0x68,FONT_SMALL,PENTA_WHITE);
	DecalFont_DrawLine("  Pistas de hielo",0x10,0x70,FONT_SMALL,POLAR_CYAN);
	DecalFont_DrawLine("  Super Turbo Pads",0x10,0x78,FONT_SMALL,TINY_GREEN);
	DecalFont_DrawLine("- Fantasmas desactivados",0x10,0x80,FONT_SMALL,PENTA_WHITE);
	DecalFont_DrawLine("- Nueva mecanica:",0x10,0x88,FONT_SMALL,PENTA_WHITE);
	DecalFont_DrawLine("  Ahora puedes obtener NADA",0x10,0x90,FONT_SMALL,JOE_COLOR);
	DecalFont_DrawLine("     Al abrir una caja",0x10,0x98,FONT_SMALL,JOE_COLOR);
	DecalFont_DrawLine("- Fasty y Herobrine removidos",0x10,0xa0,FONT_SMALL,PINSTRIPE_PALE_DARK_BLUE);
	
	MenuWrites_ServerCountry();

	// If already picked
	if(MenuFinished() == 1)
	{
		if (!initString)
		{
			strcpy(sdata->lngStrings[0x4e], countryNames[octr->serverCountry]);
			initString = true;
		}
		return;
	}

	UpdateMenu();
	NewPage_ServerCountry();
}

void ResetPsxGlobals()
{
	// unlock everything (0,1,2,3,4,5)
	for(int i = 0; i < 6; i++)
		sdata->advProgress.rewards[i] = 0xffffffff;

	// 8mb RAM expansion, for emulators that support it.
	// Needed for 3 or more players on Adv Hub
	sdata->mempack[0].lastFreeByte = 0x807ff800;
	sdata->mempack[0].endOfAllocator = 0x807ff800;

	sdata->ptrActiveMenu = 0;

	// keep running till the client gets a result,
	// DriverID is set to -1 on windows-side before this.

	for(int i = 0; i < 8; i++)
	{
		data.characterIDs[i] = 0;
		octr->boolLockedInCharacters[i] = 0;
	}
}

// should rename to EnterRoom
void StatePS1_Launch_PickRoom()
{
	DecalFont_DrawLine("Modo Clasico en Salas desde la 1 hasta 8",0x100,0x14,FONT_SMALL,JUSTIFY_CENTER|ORANGE);
	DecalFont_DrawLine("Super Turbo Pads en Salas desde la 9 hasta C",0x100,0x1c,FONT_SMALL,JUSTIFY_CENTER|TINY_GREEN);
	DecalFont_DrawLine("Pistas de hielo en Salas desde la D hasta G", 0x100, 0x24, FONT_SMALL, JUSTIFY_CENTER|POLAR_CYAN);

	MenuWrites_ServerRoom();

	// If already picked
	if(MenuFinished() == 1)
	{
		ResetPsxGlobals();
		return;
	}

	UpdateMenu();
	NewPage_ServerRoom();

	DecalFont_DrawLine("Version creada por AnzuPantzu", 0x100, 0xb4, FONT_SMALL, JUSTIFY_CENTER | OXIDE_LIGHT_GREEN);
	DecalFont_DrawLine("En colaboracion con Penta3Penguin", 0x100, 0xbc, FONT_SMALL, JUSTIFY_CENTER | PENTA_WHITE);
}

void StatePS1_Launch_Error()
{
	char str[32];

	sprintf(str, "XDELTA: %d", octr->ver_psx);
	DECOMP_DecalFont_DrawLine(str,0x100,0x98-2,FONT_SMALL,JUSTIFY_CENTER);

	sprintf(str, "CLIENTE: %d", octr->ver_pc);
	DECOMP_DecalFont_DrawLine(str,0x100,0xA0-2,FONT_SMALL,JUSTIFY_CENTER);

	sprintf(str, "SERVER: %d", octr->ver_server);
	DECOMP_DecalFont_DrawLine(str,0x100,0xA8-2,FONT_SMALL,JUSTIFY_CENTER);

	char* str2 = "Actualiza las 3 para poder jugar";
	DECOMP_DecalFont_DrawLine(str2,0x100,0xB0-2,FONT_SMALL,JUSTIFY_CENTER);

	sdata->ptrActiveMenu = 0;
}

void StatePS1_Lobby_AssignRole()
{
	menu.posX_curr = 0x70; // X position
	menu.posY_curr = 0x84;  // Y position

	if(octr->DriverID == 0)
	{
		octr->CurrState = LOBBY_HOST_TRACK_PICK;
	}

	else if (octr->DriverID > 0)
	{
		octr->CurrState = LOBBY_GUEST_TRACK_WAIT;
	}
}

void StatePS1_Lobby_HostTrackPick()
{
	MenuWrites_Tracks();

	// If already picked
	if(MenuFinished() == 1)
	{
		if(octr->levelID > TURBO_TRACK)
		{
			octr->lapID = 0;
			octr->boolLockedInLap = 1;
		}

		// do this without adding to enum,
		// cause that means changing PS1/PC
		void FakeState_Lobby_HostLapPick();
		FakeState_Lobby_HostLapPick();
		return;
	}

	PrintCharacterStats();

	UpdateMenu();
	NewPage_Tracks();
}

void FakeState_Lobby_HostLapPick()
{
	MenuWrites_Laps();

	// If already picked
	if(MenuFinished() == 1) return;

	PrintCharacterStats();

	UpdateMenu();
	NewPage_Laps();
}

void StatePS1_Lobby_GuestTrackWait()
{
	PrintCharacterStats();

	// close menu
	sdata->ptrActiveMenu = 0;

	DECOMP_DecalFont_DrawLine(
		"El host esta",
		menu.posX_curr,0xA8,FONT_SMALL,JUSTIFY_CENTER|ORANGE);

	DECOMP_DecalFont_DrawLine(
		"Seleccionando circuito",
		menu.posX_curr,0xB0,FONT_SMALL,JUSTIFY_CENTER|ORANGE);
}

void StatePS1_Lobby_CharacterPick()
{
	MenuWrites_Characters();

	// If already picked
	if(MenuFinished() == 1) return;

	PrintCharacterStats();
	PrintRecvTrack();

	UpdateMenu();
	NewPage_Characters();

	// get menu
	struct RectMenu* b = sdata->ptrActiveMenu;

	if(b != 0)
	{
		// update real-time
		data.characterIDs[0] = (8 * octr->PageNumber) + b->rowSelected;
	}
}

void StatePS1_Lobby_WaitForLoading()
{
	PrintCharacterStats();
	PrintRecvTrack();

	// waiting for all charactesr
	DECOMP_DecalFont_DrawLine(
		"Todo listo!",
		menu.posX_curr,menu.posY_curr,
		FONT_SMALL,JUSTIFY_CENTER|ORANGE);
}

static bool initRace = true;
static bool endRace = true;

void StatePS1_Lobby_StartLoading()
{
	initRace = true;
	endRace = true;
	PrintCharacterStats();
	PrintRecvTrack();

	DECOMP_DecalFont_DrawLine(
		"Iniciando.",
		menu.posX_curr,menu.posY_curr,
		FONT_SMALL,JUSTIFY_CENTER|ORANGE);

	// variable reuse, wait a few frames,
	// so screen updates with green names
	octr->CountPressX++;
	if(octr->CountPressX < FPS_DOUBLE(15)) return;

	// stop music,
	// stop "most FX", let menu FX ring
	Music_Stop();
	howl_StopAudio(1,1,0);
	sdata->unkAudioState = 0;

	struct GameTracker* gGT = sdata->gGT;

	// for battle tracks
	gGT->gameMode1 = LOADING | BATTLE_MODE;

	if(
		(octr->levelID <= TURBO_TRACK) ||
		(octr->levelID >= GEM_STONE_VALLEY)
	  )
	{
		// for all other tracks
		gGT->gameMode1 = LOADING | ARCADE_MODE;
	}

	// instant load
	//sdata->Loading.stage = 0;

	// load with flag animation
	DECOMP_MainRaceTrack_RequestLoad(octr->levelID);

	// dont kill thread,
	// it will die when loading screen covers screen
}

RECT drawTimeRECT =
{
	.x = 0xffec,
	.y = 0x46 - 3,
	.w = 0x228,
	.h = 0
};

extern unsigned int checkpointTimes[(MAX_LAPS * CPS_PER_LAP) + 1];

static void OnRaceInit()
{
	for (int i = 0; i < MAX_NUM_PLAYERS; i++)
	{
		checkpointTracker[i].currCheckpoint = 0;
		checkpointTracker[i].timer = 0;
		checkpointTracker[i].raceFinished = 0;
	}
	for (int i = 0; i < MAX_LAPS * CPS_PER_LAP; i++)
	{
		checkpointTimes[i] = 0;
	}
	sdata->gGT->drivers[0]->bestLapTime = HOURS(10);
}

void StatePS1_Game_WaitForRace()
{
	struct GameTracker* gGT = sdata->gGT;
	if (initRace)
	{
		OnRaceInit();
		initRace = false;
	}

	gGT->trafficLightsTimer = 0xf40;

	if((gGT->gameMode1 & START_OF_RACE) != 0)
		return;

	// Copy from DrawUnpluggedMsg
	int posY;
	int i;

	posY = 0x46;
	drawTimeRECT.h = 0;

	DECOMP_DecalFont_DrawLine(
		"Esperando Jugadores.",
		0x100, posY + drawTimeRECT.h,
		FONT_SMALL, (JUSTIFY_CENTER | PENTA_WHITE));

	// add for each line
	drawTimeRECT.h += 8;

	// add 3 pixels above, 3 pixels bellow
	drawTimeRECT.h += 6;

	DECOMP_RECTMENU_DrawInnerRect(
		&drawTimeRECT, 1, gGT->backBuffer->otMem.startPlusFour);

	for(i = 0; i < 8; i++)
	{
		octr->Shoot[i].boolNow = 0;
	}
}

// not really "Start", it's the trafficLights,
// and entire duration of race, should rename
void StatePS1_Game_StartRace()
{
	int i;

	for(i = 1; i < 8; i++)
	{
		if(octr->Shoot[i].boolNow != 0)
		{
			octr->Shoot[i].boolNow = 0;

			struct Driver* d = sdata->gGT->drivers[i];

			if(octr->Shoot[i].boolJuiced)
				d->numWumpas = 10;

			d->heldItemID = octr->Shoot[i].Weapon;

			// copy/paste from ShootOnCirclePress
			int weapon;
			weapon = d->heldItemID;

			// Missiles and Bombs share code,
			// Change Bomb1x, Bomb3x, Missile3x, to Missile1x
			if(
				(weapon == 1) ||
				(weapon == 10) ||
				(weapon == 11)
			)
			{
				weapon = 2;
			}

			DECOMP_VehPickupItem_ShootNow(
				d,
				weapon,
				octr->Shoot[i].flags);
		}
	}
}

static void OnRaceEnd()
{
	struct Driver ** drivers = sdata->gGT->drivers;
	bool foundRacer = false;
	for (int driverID = 1; driverID < MAX_NUM_PLAYERS; driverID++)
	{
		/* Undo wheel ghostify */
		drivers[driverID]->wheelSprites = ICONGROUP_GETICONS(sdata->gGT->iconGroup[0]);

		if (!foundRacer && octr->nameBuffer[driverID][0] && !checkpointTracker[driverID].raceFinished)
		{
			sdata->gGT->cameraDC[0].driverToFollow = drivers[driverID];
			foundRacer = true;
		}
	}
}

void StatePS1_Game_EndRace()
{
	if (endRace)
	{
		OnRaceEnd();
		endRace = false;
	}
}