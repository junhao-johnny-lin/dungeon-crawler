#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SAVE_FILE "savegame.dat"

// Structs

typedef enum { GOBLIN, ORC } MonsterType;

typedef struct Monster {
    int hp, attack, defense, speed, xp;
    MonsterType type;
} Monster;

typedef void (*RoomAction)(void*);

typedef struct Room {
    int id;
    int hasMonster, hasItem, hasTreasure, visited;
    struct Room* connections[4];
    Monster* monster;
    RoomAction action;
} Room;

typedef struct Player {
    int hp, damage, speed, defense, level, experience, expToNextLevel;
    Room* currentRoom;
} Player;

// Constants
const int baseMonsterHP = 30;
const int baseMonsterAttack = 10;
const int baseMonsterDefense = 10;
const int baseMonsterSpeed = 10;
const int baseMonsterXP = 10;

// Function declarations
Room* generateDungeon(int numRooms);
void connectRooms(Room* a, Room* b);
void displayRoom(Room* room);
void bitwiseCombat(Player* player);
void getItem(Player* player);
void levelUp(Player* player);
void displayPlayerStats(Player* player);
void saveGame(Player* player);
int loadGame(Player* player, Room* rooms);
void freeDungeon(Room* rooms, int numRooms);
void roomActionVisited(void*);

int main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL));

    int numRooms = 50;
    Room* dungeon = generateDungeon(numRooms);
    Player player = {100, 10, 10, 10, 1, 0, 100, NULL};

    // Probeer te laden, als niet bestaat start nieuw spel
    if (!loadGame(&player, dungeon)) {
        printf("Geen opgeslagen spel gevonden, start nieuw spel met %d kamers.\n", numRooms);
        player.currentRoom = &dungeon[0];
    }

    char choice;
    while (1) {
        displayRoom(player.currentRoom);
        if (player.currentRoom->hasTreasure && !player.currentRoom->hasMonster) {
            printf("\U0001F3C6 Je hebt de schat gevonden! Gefeliciteerd!\n");
            break;
        }

        if (player.currentRoom->hasMonster && player.currentRoom->action) {
            player.currentRoom->action(player.currentRoom);
            bitwiseCombat(&player);
            player.currentRoom->hasMonster = 0;
            if (player.hp <= 0) break;
        }

        if (player.currentRoom->hasItem) {
            getItem(&player);
            player.currentRoom->hasItem = 0;
        }

        printf("\nWat wil je doen?\n");
        printf("Beweeg met W (noord), A (west), S (zuid), D (oost)\n");
        printf("Druk op I om je status te bekijken\n");
        printf("Druk op X om op te slaan, Q om te stoppen\n");
        printf("Invoer: ");
        scanf(" %c", &choice);

        if (choice == 'q' || choice == 'Q') break;
        if (choice == 'x' || choice == 'X') {
            saveGame(&player);
            printf("Spel opgeslagen.\n");
            continue;
        }
        if (choice == 'i' || choice == 'I') {
            displayPlayerStats(&player);
            continue;
        }

        int dir = -1;
        if (choice == 'w' || choice == 'W') dir = 0;
        else if (choice == 'd' || choice == 'D') dir = 1;
        else if (choice == 's' || choice == 'S') dir = 2;
        else if (choice == 'a' || choice == 'A') dir = 3;

        if (dir >= 0 && player.currentRoom->connections[dir]) {
            player.currentRoom = player.currentRoom->connections[dir];
        } else {
            printf("Geen kamer in die richting.\n");
        }
    }

    freeDungeon(dungeon, numRooms);
    return 0;
}

// Genereert een lijst met kamers en verbindt ze lineair
Room* generateDungeon(int numRooms) {
    Room* rooms = malloc(sizeof(Room) * numRooms);
    for (int i = 0; i < numRooms; i++) {
        rooms[i].id = i;
        rooms[i].hasMonster = rand() % 2;
        rooms[i].hasItem = rand() % 2;
        rooms[i].hasTreasure = 0;
        rooms[i].visited = 0;
        for (int j = 0; j < 4; j++) rooms[i].connections[j] = NULL;

        if (rooms[i].hasMonster) {
            rooms[i].monster = malloc(sizeof(Monster));
            float scale = 1 + 0.1f * i; // +10% per kamer
            rooms[i].monster->hp = (int)(baseMonsterHP * scale);
            rooms[i].monster->attack = (int)(baseMonsterAttack * scale);
            rooms[i].monster->defense = (int)(baseMonsterDefense * scale);
            rooms[i].monster->speed = (int)(baseMonsterSpeed * scale);
            rooms[i].monster->xp = (int)(baseMonsterXP * scale);
            rooms[i].monster->type = (i % 2 == 0) ? GOBLIN : ORC;
            rooms[i].action = roomActionVisited;
        } else {
            rooms[i].monster = NULL;
            rooms[i].action = NULL;
        }
    }
    rooms[numRooms - 1].hasTreasure = 1;
    for (int i = 0; i < numRooms - 1; i++) connectRooms(&rooms[i], &rooms[i + 1]);
    return rooms;
}

// Verbindt twee kamers bidirectioneel op een willekeurige richting
void connectRooms(Room* a, Room* b) {
    int dir = rand() % 4;
    a->connections[dir] = b;
    b->connections[(dir + 2) % 4] = a;
}

// Toont info over kamer en eventuele monster/item/schat
void displayRoom(Room* room) {
    printf("\n🔹 Kamer %d\n", room->id);
    if (room->hasMonster && room->monster) {
        printf("Monster (%s): HP=%d, ATK=%d, DEF=%d, SPD=%d\n",
               room->monster->type == GOBLIN ? "Goblin" : "Orc",
               room->monster->hp,
               room->monster->attack,
               room->monster->defense,
               room->monster->speed);
    }
    if (room->hasItem) printf("✨ Je ziet een item liggen.\n");
    if (room->hasTreasure) printf("💰 Er ligt een schat!\n");
}

// Gevechtssysteem met bitpatroon en ontwijken op basis van speed
void bitwiseCombat(Player* player) {
    Monster* m = player->currentRoom->monster;
    int round = 1;

    while (player->hp > 0 && m->hp > 0) {
        int pattern = rand() % 16;

        // Toon aanvalsvolgorde (bitpatroon)
        printf("\n🔫 Aanvalsvolgorde ronde %d:\n", round++);
        printf("Patroon bits: ");
        for (int i = 3; i >= 0; i--) {
            printf("%d", ((pattern >> i) & 1));
        }
        printf("\n");

        // Voer 4 beurten uit volgens bitpatroon
        for (int i = 3; i >= 0; i--) {
            if (player->hp <= 0 || m->hp <= 0) break;

            int bit = (pattern >> i) & 1;

            if (bit == 1) {
                // Speler valt aan, monster kan ontwijken
                float speedDiff = player->speed - m->speed;
                float dodgeChance = 0.0f;
                if (speedDiff < 0) {
                    dodgeChance = -speedDiff * 0.05f;
                    if (dodgeChance > 0.5f) dodgeChance = 0.5f;
                }

                printf("Speler valt aan... ");

                if ((float)rand() / RAND_MAX < dodgeChance) {
                    printf("%s ontwijkt!\n", (m->type == GOBLIN) ? "Goblin" : "Orc");
                } else {
                    int dmg = player->damage - m->defense;
                    if (dmg < 1) dmg = 1;
                    m->hp -= dmg;
                    printf("%s krijgt %d schade. %s heeft nu: %d HP\n",
                           (m->type == GOBLIN) ? "Goblin" : "Orc",
                           dmg,
                           (m->type == GOBLIN) ? "Goblin" : "Orc",
                           m->hp);
                }
            } else {
                // Monster valt aan, speler kan ontwijken
                float speedDiff = m->speed - player->speed;
                float dodgeChance = 0.0f;
                if (speedDiff < 0) {
                    dodgeChance = -speedDiff * 0.05f;
                    if (dodgeChance > 0.5f) dodgeChance = 0.5f;
                }

                printf("%s valt aan... ", (m->type == GOBLIN) ? "Goblin" : "Orc");

                if ((float)rand() / RAND_MAX < dodgeChance) {
                    printf("Speler ontwijkt!\n");
                } else {
                    int dmg = m->attack - player->defense;
                    if (dmg < 1) dmg = 1;
                    player->hp -= dmg;
                    printf("Speler krijgt %d schade. Speler heeft nu: %d HP\n", dmg, player->hp);
                }
            }
        }
        printf("-----------------------------\n");
    }

    // Na gevecht
    if (player->hp > 0 && m->hp <= 0) {
        printf("✅ %s verslagen. +%d XP\n", (m->type == GOBLIN) ? "Goblin" : "Orc", m->xp);
        player->experience += m->xp;

        // +1 op alle stats na verslaan monster
        player->hp += 1;
        player->damage += 1;
        player->defense += 1;
        player->speed += 1;
        printf("🎉 Je krijgt +1 op alle stats!\n");
        printf("Je nieuwe stats: HP=%d | Damage=%d | Defense=%d | Speed=%d\n",
               player->hp, player->damage, player->defense, player->speed);

        // Level up check
        while (player->experience >= player->expToNextLevel) levelUp(player);
    } else if (player->hp <= 0) {
        printf("☠️  Je bent verslagen...\n");
    }
}

// Speler krijgt een willekeurig item
void getItem(Player* player) {
    int t = rand() % 4;
    if (t == 0) { player->hp += 20; printf("❤️ Je vond een genezend item! (+20 HP)\n"); }
    else if (t == 1) { player->damage += 5; printf("⚔️ Je kracht neemt toe! (+5 Damage)\n"); }
    else if (t == 2) { player->defense += 5; printf("🛡️ Je verdediging stijgt! (+5 Defense)\n"); }
    else { player->speed += 5; printf("💨 Je snelheid neemt toe! (+5 Speed)\n"); }
}

// Player levelt up en krijgt stats boost
void levelUp(Player* player) {
    player->level++;
    player->hp += 10;
    player->damage += 5;
    player->defense += 5;
    player->speed += 5;
    player->experience -= player->expToNextLevel;
    player->expToNextLevel += 10;
    printf("🌟 Level up! Nu level %d\n", player->level);
}

// Laat speler stats zien
void displayPlayerStats(Player* p) {
    printf("HP: %d | Damage: %d | Defense: %d | Speed: %d | Level: %d | XP: %d/%d\n",
           p->hp, p->damage, p->defense, p->speed, p->level, p->experience, p->expToNextLevel);
}

// Opslaan van spel
void saveGame(Player* p) {
    FILE* f = fopen(SAVE_FILE, "wb");
    if (!f) return;
    fwrite(p, sizeof(Player), 1, f);
    int id = p->currentRoom->id;
    fwrite(&id, sizeof(int), 1, f);
    fclose(f);
}

// Laden van spel
int loadGame(Player* p, Room* rooms) {
    FILE* f = fopen(SAVE_FILE, "rb");
    if (!f) return 0;
    fread(p, sizeof(Player), 1, f);
    int id;
    fread(&id, sizeof(int), 1, f);
    if (id < 0) id = 0;
    if (id >= 50) id = 0; // veiligheid
    p->currentRoom = &rooms[id];
    fclose(f);
    return 1;
}

// Vrijgeven van geheugengebruik voor dungeon
void freeDungeon(Room* rooms, int numRooms) {
    for (int i = 0; i < numRooms; i++) {
        if (rooms[i].monster) free(rooms[i].monster);
    }
    free(rooms);
}

// Actie bij betreden kamer (voor eerste bezoek)
void roomActionVisited(void* r) {
    Room* room = (Room*)r;
    if (!room->visited) {
        room->visited = 1;
        printf("Je betreedt deze kamer voor het eerst.\n");
    }
}
