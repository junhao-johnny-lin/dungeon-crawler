#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MIN_ROOMS 20
#define MAX_ROOMS 30
#define SAVE_FILE "savegame.dat"

typedef struct Room {
    int id;
    int hasMonster;
    int hasItem;
    int hasTreasure;
    struct Room* connections[4]; // [0]=N, [1]=E, [2]=S, [3]=W
    int monsterHP;
    int monsterAttack;
    int monsterXP;
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
void displayPlayerStats(Player* player);

// Static variables to keep track of initial monster stats
static int baseMonsterHP = 30;
static int baseMonsterAttack = 10;
static int baseMonsterXP = 10;

int main() {
    srand((unsigned int)time(NULL));
    int numRooms = MIN_ROOMS + rand() % (MAX_ROOMS - MIN_ROOMS + 1); // Random number of rooms between MIN_ROOMS and MAX_ROOMS
    Player player = {10, 10, 10, 10, 10, 0, 1,  100, NULL}; // Initialize experience to 0 and expToNextLevel to 100
    Room* dungeon = generateDungeon(numRooms);

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

        printf("\nWat wil je doen?\n");
        printf("Beweeg met W (noord), A (west), S (zuid), D (oost)\n");
        printf("Druk op I om je status te bekijken\n");
        printf("Druk op X om op te slaan, Q om te stoppen\n");
        printf("Invoer: ");
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
        if (choice == 'w' || choice == 'W') dir = 0;      // noord
        else if (choice == 'd' || choice == 'D') dir = 1; // oost
        else if (choice == 's' || choice == 'S') dir = 2; // zuid
        else if (choice == 'a' || choice == 'A') dir = 3; // west

        if (dir >= 0 && player.currentRoom->connections[dir]) {
            player.currentRoom = player.currentRoom->connections[dir];
            // Reset monster stats for new room
            if (player.currentRoom->hasMonster) {
                player.currentRoom->monsterHP = baseMonsterHP;
                player.currentRoom->monsterAttack = baseMonsterAttack;
                player.currentRoom->monsterXP = baseMonsterXP;
            }
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
        rooms[i].monsterHP = baseMonsterHP;
        rooms[i].monsterAttack = baseMonsterAttack;
        rooms[i].monsterXP = baseMonsterXP;
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
    printf("\n⚔️  Gevecht gestart!\n");

    // Display initial status
    printf("\n--- Status ---\n");
    printf("👤 Speler - HP: %d | Damage: %d | Level: %d | Experience: %d/%d\n", player->hp, player->damage, player->level, player->experience, player->expToNextLevel);
    printf("👹 Monster - HP: %d | Attack: %d | XP: %d\n", player->currentRoom->monsterHP, player->currentRoom->monsterAttack, player->currentRoom->monsterXP);

    while (player->currentRoom->monsterHP > 0 && player->hp > 0) {
        printf("\n----------------------------------\n"); // Separator line for each turn

        // Monster attack
        int dodgeChance = abs(player->speed - player->currentRoom->monsterAttack) * 10;
        if (rand() % 100 > dodgeChance) {
            int damage = player->currentRoom->monsterAttack - player->defense;
            if (damage < 0) damage = 0;
            player->hp -= damage;
            printf("💥 Monster valt aan! Je wordt getroffen voor %d HP. Je HP: %d\n", damage, player->hp);
        } else {
            printf("💨 Je ontwijkt de aanval van het monster! Je HP: %d\n", player->hp);
        }

        // Check if player is still alive
        if (player->hp <= 0) {
            printf("☠️  Je bent gestorven...\n");
            printf("💀 Je bent getroffen door het monster!\n");
            printf("☠️ Je bent gestorven. Game over.\n");
            break;
        }

        // Player attack
        player->currentRoom->monsterHP -= player->damage;
        printf("👊 Jij valt aan! Monster heeft nu %d HP\n", player->currentRoom->monsterHP);

        // Check if monster is still alive
        if (player->currentRoom->monsterHP <= 0) {
            printf("✅ Monster verslagen!\n");
            printf("🎉 Je krijgt %d ervaringpunten!\n", player->currentRoom->monsterXP);
            player->experience += player->currentRoom->monsterXP;
            if (player->experience >= player->expToNextLevel) {
                levelUp(player);
            }
            break;
        }
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

void displayPlayerStats(Player* player) {
    printf("\n--- Speler Status ---\n");
    printf("👤 Speler - HP: %d | Damage: %d | Level: %d | Experience: %d/%d\n", player->hp, player->damage, player->level, player->experience, player->expToNextLevel);
}