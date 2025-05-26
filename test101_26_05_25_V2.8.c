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

int main() {
    srand((unsigned int)time(NULL));

    int numRooms = 50;
    Room* dungeon = generateDungeon(numRooms);
    Player player = {100, 10, 10, 10, 1, 0, 100, NULL};

    if (!loadGame(&player, dungeon)) {
        printf("🔹 Geen opgeslagen spel gevonden. Nieuw spel wordt gestart.\n");
        player.currentRoom = &dungeon[0];
    } else {
        printf("🔹 Spel geladen. Welkom terug, Speler!\n");
    }

    char choice;
    while (1) {
        displayRoom(player.currentRoom);

        if (player.currentRoom->hasTreasure && !player.currentRoom->hasMonster) {
            printf("💰 Speler heeft de schat gevonden! Gefeliciteerd!\n");
            break;
        }

        if (player.currentRoom->hasMonster && player.currentRoom->action) {
            player.currentRoom->action(player.currentRoom);
            bitwiseCombat(&player);
            if (player.hp <= 0) break;
            player.currentRoom->hasMonster = 0;
        }

        if (player.currentRoom->hasItem) {
            getItem(&player);
            player.currentRoom->hasItem = 0;
        }

        printf("\n🔹 Wat wil Speler doen?\nBeweeg met W (noord), A (west), S (zuid), D (oost)\nStatus bekijken: I\nOpslaan: X\nStoppen: Q\nInvoer: ");
        scanf(" %c", &choice);

        if (choice == 'q' || choice == 'Q') break;
        if (choice == 'x' || choice == 'X') {
            saveGame(&player);
            printf("💾 Spel opgeslagen.\n");
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
            printf("❌ Geen kamer in die richting.\n");
        }
    }

    freeDungeon(dungeon, numRooms);
    return 0;
}

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
            float scale = 1 + 0.1f * i;
            rooms[i].monster->hp = baseMonsterHP * scale;
            rooms[i].monster->attack = baseMonsterAttack * scale;
            rooms[i].monster->defense = baseMonsterDefense * scale;
            rooms[i].monster->speed = baseMonsterSpeed * scale;
            rooms[i].monster->xp = baseMonsterXP * scale;
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

void connectRooms(Room* a, Room* b) {
    int dir = rand() % 4;
    a->connections[dir] = b;
    b->connections[(dir + 2) % 4] = a;
}

void displayRoom(Room* room) {
    printf("\n🔹 --- Kamer %d ---\n", room->id);
    if (room->hasMonster && room->monster) {
        printf("👹 %s aanwezig: HP=%d, ATK=%d\n",
               room->monster->type == GOBLIN ? "Goblin" : "Orc",
               room->monster->hp, room->monster->attack);
    }
    if (room->hasItem) printf("✨ Speler vindt een item.\n");
    if (room->hasTreasure) printf("💰 Er ligt een schat!\n");
}

void bitwiseCombat(Player* player) {
    Monster* m = player->currentRoom->monster;
    const char* monsterName = m->type == GOBLIN ? "Goblin" : "Orc";
    int round = 1;

    while (player->hp > 0 && m->hp > 0) {
        int pattern = rand() % 16;
        printf("\n🔹 Aanvalsvolgorde (Beurt %02d): Bitpatroon: ", round++);
        for (int i = 3; i >= 0; i--) printf("%d", (pattern >> i) & 1);
        printf("\n");

        for (int i = 3; i >= 0; i--) {
            if (player->hp <= 0 || m->hp <= 0) break;

            if ((pattern >> i) & 1) {
                int speedDiff = player->speed - m->speed;
                float dodgeChance = speedDiff > 0 ? speedDiff * 0.05f : 0.0f;
                if (dodgeChance > 0.5f) dodgeChance = 0.5f;

                if ((float)rand() / RAND_MAX < dodgeChance) {
                    printf("🛡️ %s ontwijkt de aanval van Speler!\n", monsterName);
                } else {
                    int dmg = player->damage - m->defense;
                    if (dmg < 1) dmg = 1;
                    m->hp -= dmg;
                    printf("⚔️ Speler doet %d schade aan %s. %s HP: %d\n", dmg, monsterName, monsterName, m->hp);
                }
            } else {
                int speedDiff = m->speed - player->speed;
                float dodgeChance = speedDiff > 0 ? speedDiff * 0.05f : 0.0f;
                if (dodgeChance > 0.5f) dodgeChance = 0.5f;

                if ((float)rand() / RAND_MAX < dodgeChance) {
                    printf("🛡️ Speler ontwijkt de aanval van %s!\n", monsterName);
                } else {
                    int dmg = m->attack - player->defense;
                    if (dmg < 1) dmg = 1;
                    player->hp -= dmg;
                    printf("💥 %s doet %d schade aan Speler. Speler HP: %d\n", monsterName, dmg, player->hp);
                }
            }
        }
        printf("-----------------------------\n");
    }

    if (player->hp > 0) {
        printf("✅ Speler verslaat de %s. +%d XP\n", monsterName, m->xp);
        player->experience += m->xp;
        player->hp += 1;
        player->damage += 1;
        player->defense += 1;
        player->speed += 1;
        printf("📈 Speler wordt sterker! +1 op alle statistieken:\n");
        printf("  +1 HP, +1 Damage, +1 Defense, +1 Speed\n");
        displayPlayerStats(player);
        while (player->experience >= player->expToNextLevel) levelUp(player);
    } else {
        printf("☠️  Speler is verslagen...\n");
    }
}

void getItem(Player* player) {
    int t = rand() % 4;
    if (t == 0) { player->hp += 20; printf("❤️ Speler krijgt +20 HP.\n"); }
    else if (t == 1) { player->damage += 5; printf("🗡️ Speler krijgt +5 Damage.\n"); }
    else if (t == 2) { player->defense += 5; printf("🛡️ Speler krijgt +5 Defense.\n"); }
    else { player->speed += 5; printf("⚡ Speler krijgt +5 Speed.\n"); }
}

void levelUp(Player* player) {
    player->level++;
    player->hp += 10;
    player->damage += 5;
    player->defense += 5;
    player->speed += 5;
    player->experience -= player->expToNextLevel;
    player->expToNextLevel += 10;
    printf("🌟 Speler bereikt level %d! Statistieken verhoogd:\n", player->level);
    printf("  +10 HP, +5 Damage, +5 Defense, +5 Speed\n");
    displayPlayerStats(player);
}

void displayPlayerStats(Player* p) {
    printf("📊 Speler Stats:\n");
    printf("  HP: %d\n", p->hp);
    printf("  Damage: %d\n", p->damage);
    printf("  Defense: %d\n", p->defense);
    printf("  Speed: %d\n", p->speed);
    printf("  Level: %d\n", p->level);
    printf("  XP: %d/%d\n", p->experience, p->expToNextLevel);
}

void saveGame(Player* p) {
    FILE* f = fopen(SAVE_FILE, "wb");
    if (!f) return;
    fwrite(p, sizeof(Player), 1, f);
    int id = p->currentRoom->id;
    fwrite(&id, sizeof(int), 1, f);
    fclose(f);
}

int loadGame(Player* p, Room* rooms) {
    FILE* f = fopen(SAVE_FILE, "rb");
    if (!f) return 0;
    fread(p, sizeof(Player), 1, f);
    int id;
    fread(&id, sizeof(int), 1, f);
    p->currentRoom = &rooms[id];
    fclose(f);
    return 1;
}

void freeDungeon(Room* rooms, int numRooms) {
    for (int i = 0; i < numRooms; i++) {
        if (rooms[i].monster) free(rooms[i].monster);
    }
    free(rooms);
}

void roomActionVisited(void* r) {
    Room* room = (Room*)r;
    if (!room->visited) {
        room->visited = 1;
        printf("🔹 Speler betreedt deze kamer voor het eerst.\n");
    }
}
