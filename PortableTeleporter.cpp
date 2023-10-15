// PortableTeleporter.cpp

// In this plug-in, only the minimal to make it functional has been done.
// Checking the capture, flag grab and flag dropped events could have been done too.
// 
// To be noted: This plug-in is not over-engineered like some other works of mine.
// Players simply have to have the tele flag and have a single save position to teleport during their entire game session.
//

#include "bzfsAPI.h"
#include "../src/bzfs/bzfs.h"
#include "../src/bzfs/GameKeeper.h"

int SetPlayerDead(int playerID) {
    int status = 0;
    GameKeeper::Player *p = GameKeeper::Player::getPlayerByIndex(playerID);
    if (p == NULL) {
        status= -1;
    } else {
        if (p->player.isObserver()) {
            status= -2;
        } else {
            if (p->player.isAlive()) {
                status=1;
                p->player.setDead();
                p->player.setRestartOnBase(false);
            } else {
                status=2;
            }
        }
    }
    return status;
}


void ForcePlayerSpawn(int playerID) {
    playerAlive(playerID);
}


// Utility functions.
int checkRange(int min, int max, int amount);
int checkPlayerSlot(int player);

class PortableTeleporter : public bz_Plugin, bz_CustomSlashCommandHandler
{
public:
  const char* Name(){return "PortableTeleporter";}
  void Init ( const char* /*config*/ );
  void Event(bz_EventData *eventData );
  void Cleanup ( void );
  bool SlashCommand ( int playerID, bz_ApiString command, bz_ApiString message, bz_APIStringList* params );
  int spawnsaved[200];
  float playerPos[200][4];

};

BZ_PLUGIN(PortableTeleporter)

void PortableTeleporter::Init (const char* commandLine) {
    bz_RegisterCustomFlag("PT", "Portable Teleporter", "Use /savepos to save a spot and /teleport to go there.", 0, eGoodFlag);
    for(int i=0;i<=199;i++) {
        spawnsaved[i]= -1;
        playerPos[i][0]=0.0;
        playerPos[i][1]=0.0;
        playerPos[i][2]=0.0;
        playerPos[i][3]=0.0;
    }
  bz_registerCustomSlashCommand ( "savepos", this );
  bz_registerCustomSlashCommand ( "teleport", this );
  Register(bz_ePlayerJoinEvent);
  Register(bz_ePlayerPartEvent);
  Register(bz_eGetPlayerSpawnPosEvent);
}

void PortableTeleporter::Cleanup (void) {
  bz_removeCustomSlashCommand ( "savepos" );
  bz_removeCustomSlashCommand ( "teleport" );
  Flush();
}

bool PortableTeleporter::SlashCommand ( int playerID, bz_ApiString command, bz_ApiString message, bz_APIStringList* params ) {
  if (command == "savepos") {
    if (checkPlayerSlot(playerID) == 1) {
        bz_BasePlayerRecord *playRec = bz_getPlayerByIndex ( playerID );
        if (playRec != NULL) {
            //bz_debugMessagef(0, "Flag: %s flagID: %d\n", playRec->currentFlag.c_str(), playRec->currentFlagID);
            if ((playRec->spawned) && (playRec->currentFlag == "Portable Teleporter (+PT)")) {
                //(strcmp(playRec->currentFlag.c_str(),"PT")==0)) {
                playerPos[playerID][0]= playRec->lastKnownState.pos[0];
                playerPos[playerID][1]= playRec->lastKnownState.pos[1];
                playerPos[playerID][2]= playRec->lastKnownState.pos[2];
                playerPos[playerID][3]= playRec->lastKnownState.rotation;
                spawnsaved[playerID] = 1;
                bz_sendTextMessage(BZ_SERVER,playerID,"Spawn Position Saved.");
            } else {
                bz_sendTextMessage(BZ_SERVER,playerID,"You can't save your position without a Portable Teleporter.");
                //bz_sendTextMessage(BZ_SERVER,playerID,".");
            }
        }
        bz_freePlayerRecord(playRec);
    } else {
        bz_sendTextMessage(BZ_SERVER,playerID,"ERROR: Invalid player slot.");
        bz_sendTextMessage(BZ_SERVER,playerID,"ERROR: Unable to save position.");
    }  
    return true;
  } 
  if (command == "teleport") {
        if (checkPlayerSlot(playerID) == 1) {
            if (spawnsaved[playerID] == 1) {
                bz_BasePlayerRecord *playRec = bz_getPlayerByIndex ( playerID );
                if (playRec != NULL) {
                    if ((playRec->spawned) && (playRec->currentFlag == "Portable Teleporter (+PT)")){
                        SetPlayerDead(playerID);
                        ForcePlayerSpawn(playerID);
                    } else {
                        bz_sendTextMessage(BZ_SERVER,playerID,"You need a Portable Teleporter if you want to do that.");
                    }
                }
                bz_freePlayerRecord(playRec);
                //spawnsaved[playerID] = -1;
                // Could be done differently, let's do it in spawns instead.
                // In theory I could add in another option to make semi-perma save spawn positions.
            } else {
                bz_sendTextMessage(BZ_SERVER,playerID,"You need to save a position first with \"/savepos\"");
            }
        } else {
            bz_sendTextMessage(BZ_SERVER,playerID,"ERROR: Invalid player slot.");
            bz_sendTextMessage(BZ_SERVER,playerID,"ERROR: Teleport function not usable.");
        }
        return true;
    }
  return false;
}

void PortableTeleporter::Event(bz_EventData *eventData ){
  switch (eventData->eventType) {
    case bz_ePlayerJoinEvent: {
      bz_PlayerJoinPartEventData_V1* joinData = (bz_PlayerJoinPartEventData_V1*)eventData;
      int player = joinData->playerID;
      if (checkPlayerSlot(player) == 1) {
        // We're assuming a new game session, reset data.
        spawnsaved[player]= -1;
        for(int i=0;i<=3;i++) {
            playerPos[player][i]=0.0;
        }
      }
    }break;

    case bz_ePlayerPartEvent: {
      bz_PlayerJoinPartEventData_V1* partData = (bz_PlayerJoinPartEventData_V1*)eventData;
      int player = partData->playerID;
      if (checkPlayerSlot(player) == 1) {
        // We're assuming the end of a game session, reset data.
        spawnsaved[player]= -1;
        for(int i=0;i<=3;i++) {
            playerPos[player][i]=0.0;
        }
      }
    }break;

    case bz_eGetPlayerSpawnPosEvent: {
      bz_GetPlayerSpawnPosEventData_V1* spawnPosData = (bz_GetPlayerSpawnPosEventData_V1*)eventData;
      int player = spawnPosData->playerID;
      if (checkPlayerSlot(player) == 1) {
        if (spawnsaved[player] == 1) {
            spawnPosData->handled = true;
            spawnPosData->pos[0]= playerPos[player][0];
            spawnPosData->pos[1]= playerPos[player][1];
            spawnPosData->pos[2]= (playerPos[player][2] + 0.5); // To prevent spawning into the meshes.
            spawnPosData->rot   = playerPos[player][3];
            spawnsaved[player]= -1;
            // Resetting the data can be done be changing the spawnable value to false.
        }
      }
    }break;

    default:{ 
    }break;
  }
}


int checkRange(int min, int max, int amount) {
    int num = 0;
    if ((amount >= min) && (amount <= max)) {
        num = 1;
    } else if ((amount < min) || (amount > max)) {
        num = 0;
    } else {
        num = -1;
    }
    return num;
}

int checkPlayerSlot(int player) {
    return checkRange(0,199,player); // 199 because of array.
}
