#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MAX_ROOMS 20
#define SAVE_FILE "savegame.dat"

// Bitwise attack types
#define ATTACK_SLASH 0x01
#define ATTACK_FIRE  0x02
#define ATTACK_MAGIC 0x04

typedef struct Room {
    int id;
    int hasMonster;
    int hasItem;
    int hasTreasure;
    struct Room* connections[4]; // [0]=N, [1]=E, [2]=S, [3]=W
} Room;

typedef struct Player {
    int hp;
    int strength;
    int damage;
    int speed;
    int defense;
    int level;
    int experience;
    int expToNextLevel;
    Room* currentRoom;
} Player;

// Function prototypes
Room* generateDungeon(int numRooms);
void connectRooms(Room* a, Room* b);
void displayRoom(Room* room);
void battle(Player* player);
void getItem(Player* player);
void saveGame(Player* player);
int loadGame(Player* player, Room* rooms);
void levelUp(Player* player);
int isMonsterSleeping();
int tryToSneakPast(Player* player);

int main() {
    srand((unsigned int)time(NULL));
    Player player = {10, 10, 10, 10, 0, 1, 0, 100, NULL}; // Initialize experience to 0 and expToNextLevel to 100
    Room* dungeon = generateDungeon(MAX_ROOMS);

    if (!loadGame(&player, dungeon)) {
        player.currentRoom = &dungeon[0];
    }

    char choice;
    while (1) {
        displayRoom(player.currentRoom);

        if (player.currentRoom->hasTreasure) {
            printf("🏆 Je hebt de schat gevonden! Gefeliciteerd!\n");
            break;
        }

        if (player.currentRoom->hasMonster) {
            if (isMonsterSleeping()) {
                printf("👹 Er is een slapend monster in deze kamer! Wil je vechten (F) of proberen te ontwijken (S)?\n");
                scanf(" %c", &choice);
                if (choice == 'f' || choice == 'F') {
                    battle(&player);
                    player.currentRoom->hasMonster = 0;
                    if (player.hp <= 0) {
                        printf("☠️  Je bent gestorven. Game over.\n");
                        break;
                    }
                } else if (choice == 's' || choice == 'S') {
                    if (tryToSneakPast(&player)) {
                        printf("💤 Je slaapt het gevecht over.\n");
                    } else {
                        printf("⚠️  Je kunt het monster niet ontwijken! Je moet vechten.\n");
                        battle(&player);
                        player.currentRoom->hasMonster = 0;
                        if (player.hp <= 0) {
                            printf("☠️  Je bent gestorven. Game over.\n");
                            break;
                        }
                    }
                }
            } else {
                printf("👹 Er is een monster in deze kamer! Je moet vechten.\n");
                battle(&player);
                player.currentRoom->hasMonster = 0;
                if (player.hp <= 0) {
                    printf("☠️  Je bent gestorven. Game over.\n");
                    break;
                }
            }
        }

        if (player.currentRoom->hasItem) {
            getItem(&player);
            player.currentRoom->hasItem = 0;
        }

        printf("\nBeweeg met W (noord), A (west), S (zuid), D (oost)\n");
        printf("Of druk op X om op te slaan, Q om te stoppen\n");
        printf("Invoer: ");
        scanf(" %c", &choice);

        if (choice == 'q' || choice == 'Q') break;
        if (choice == 'x' || choice == 'X') {
            saveGame(&player);
            printf("💾 Spel opgeslagen.\n");
            continue;
        }

        int dir = -1;
        if (choice == 'w' || choice == 'W') dir = 0;      // noord
        else if (choice == 'd' || choice == 'D') dir = 1; // oost
        else if (choice == 's' || choice == 'S') dir = 2; // zuid
        else if (choice == 'a' || choice == 'A') dir = 3; // west

        if (dir >= 0 && player.currentRoom->connections[dir]) {
            player.currentRoom = player.currentRoom->connections[dir];
        } else {
            printf("❌ Geen kamer in die richting.\n");
        }
    }

    free(dungeon);
    return 0;
}

Room* generateDungeon(int numRooms) {
    Room* rooms = malloc(sizeof(Room) * numRooms);
    for (int i = 0; i < numRooms; i++) {
        rooms[i].id = i;
        rooms[i].hasMonster = rand() % 2;
        rooms[i].hasItem = rand() % 2;
        rooms[i].hasTreasure = 0;
        for (int j = 0; j < 4; j++) {
            rooms[i].connections[j] = NULL;
        }
    }

    rooms[numRooms - 1].hasTreasure = 1;

    for (int i = 0; i < numRooms - 1; i++) {
        connectRooms(&rooms[i], &rooms[i + 1]);
    }

    return rooms;
}

void connectRooms(Room* a, Room* b) {
    int dir = rand() % 4;
    a->connections[dir] = b;
    b->connections[(dir + 2) % 4] = a;
}

void displayRoom(Room* room) {
    printf("\n🔹 Kamer %d\n", room->id);
    if (room->hasMonster)  printf("👹 Er is een monster in deze kamer!\n");
    if (room->hasItem)     printf("✨ Je ziet een item liggen.\n");
    if (room->hasTreasure) printf("💰 De schat ligt hier!\n");
}

int tryToSneakPast(Player* player) {
    // 50% chance to sneak past a sleeping monster
    return rand() % 2;
}

int isMonsterSleeping() {
    // 50% chance the monster is sleeping
    return rand() % 2;
}

void battle(Player* player) {
    static int monsterHP = 30;
    static int monsterAttack = ATTACK_SLASH | ATTACK_FIRE;
    static int monsterXP = 10;

    printf("\n⚔️  Gevecht gestart!\n");

    while (monsterHP > 0 && player->hp > 0) {
        printf("\n--- Status ---\n");
        printf("👤 Speler - HP: %d | Damage: %d | Level: %d | Experience: %d/%d\n", player->hp, player->damage, player->level, player->experience, player->expToNextLevel);
        printf("👹 Monster - HP: %d\n", monsterHP);

        // Monster kiest willekeurig een aanval
        int attackType = rand() % 2;
        int dodgeChance = abs(player->speed - monsterAttack) * 10;
        int monsterDamage = 0;

        if (attackType == 0 && (monsterAttack & ATTACK_FIRE)) {
            monsterDamage = 8;
            printf("🔥 Monster gebruikt vuur!\n");
        } else if (attackType == 1 && (monsterAttack & ATTACK_SLASH)) {
            monsterDamage = 5;
            printf("🗡️  Monster valt met zwaard aan!\n");
        }

        if (rand() % 100 > dodgeChance) {
            int damage = monsterDamage - player->defense;
            if (damage < 0) damage = 0;
            player->hp -= damage;
            printf("💥 Je wordt getroffen! (-%d HP)\n", damage);
        } else {
            printf("💨 Je ontwijkt de aanval!\n");
        }

        // Speler aanval
        monsterHP -= player->damage;
        if (monsterHP < 0) monsterHP = 0;
        printf("👊 Jij valt aan! Monster heeft nu %d HP\n", monsterHP);
    }

    if (player->hp > 0) {
        printf("✅ Monster verslagen!\n");
        player->experience += monsterXP;
        if (player->experience >= player->expToNextLevel) {
            levelUp(player);
        }
        monsterHP += 3; // 10% more HP
        monsterAttack += 1; // 10% more strength
        monsterXP += 1; // 10% more XP
    } else {
        printf("☠️  Je bent verslagen...\n");
    }
}

void getItem(Player* player) {
    int boost = rand() % 2;
    if (boost == 0) {
        player->hp += 20;
        printf("❤️ Je vond een genezend item! (+20 HP)\n");
    } else {
        player->damage += 5;
        printf("💪 Je vond een krachtitem! (+5 schade)\n");
    }
}

void saveGame(Player* player) {
    FILE* f = fopen(SAVE_FILE, "wb");
    if (f) {
        fwrite(&player->hp, sizeof(int), 1, f);
        fwrite(&player->strength, sizeof(int), 1, f);
        fwrite(&player->damage, sizeof(int), 1, f);
        fwrite(&player->speed, sizeof(int), 1, f);
        fwrite(&player->defense, sizeof(int), 1, f);
        fwrite(&player->level, sizeof(int), 1, f);
        fwrite(&player->experience, sizeof(int), 1, f);
        fwrite(&player->expToNextLevel, sizeof(int), 1, f);
        int id = player->currentRoom->id;
        fwrite(&id, sizeof(int), 1, f);
        fclose(f);
    }
}

int loadGame(Player* player, Room* rooms) {
    FILE* f = fopen(SAVE_FILE, "rb");
    if (f == NULL) {
        // No saved game file found
        return 0;
    }

    fread(&player->hp, sizeof(int), 1, f);
    fread(&player->strength, sizeof(int), 1, f);
    fread(&player->damage, sizeof(int), 1, f);
    fread(&player->speed, sizeof(int), 1, f);
    fread(&player->defense, sizeof(int), 1, f);
    fread(&player->level, sizeof(int), 1, f);
    fread(&player->experience, sizeof(int), 1, f);
    fread(&player->expToNextLevel, sizeof(int), 1, f);
    int id;
    fread(&id, sizeof(int), 1, f);
    player->currentRoom = &rooms[id];
    fclose(f);
    printf("💾 Spel geladen vanaf kamer %d\n", id);
    return 1;
}

void levelUp(Player* player) {
    player->level++;
    player->hp += 5;
    player->strength += 5;
    player->damage += 5;
    player->experience = 0;
    player->expToNextLevel = (int)(player->expToNextLevel * 1.25);
    printf("🌟 Je bent gelevelupt! Nieuwe level: %d\n", player->level);
}