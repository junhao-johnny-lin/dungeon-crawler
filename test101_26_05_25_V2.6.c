#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MIN_ROOMS 20
#define MAX_ROOMS 100
#define SAVE_FILE "savegame.dat"

typedef struct Monster {
    int hp;
    int attack;
    int defense;
    int speed;
    int xp;
} Monster;

typedef struct Room {
    int id;
    int hasMonster;
    int hasItem;
    int hasTreasure;
    struct Room* connections[4]; // [0]=N, [1]=E, [2]=S, [3]=W
    Monster* monster;
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
static int baseMonsterDefense = 10; // Added missing definition
static int baseMonsterSpeed = 10;   // Added missing definition
static int baseMonsterXP = 10;

int main() {
    srand((unsigned int)time(NULL));
    int numRooms = MIN_ROOMS + rand() % (MAX_ROOMS - MIN_ROOMS + 1); // Random number of rooms between MIN_ROOMS and MAX_ROOMS
    Player player = {100, 10, 10, 10, 10, 0, 0, 100, NULL}; // Initialize experience to 0 and expToNextLevel to 100
    Room* dungeon = generateDungeon(numRooms);

    if (!loadGame(&player, dungeon)) {
        player.currentRoom = &dungeon[0];
    }

    char choice;
    while (1) {
        displayRoom(player.currentRoom);

        if (player.currentRoom->hasTreasure) {
            if (player.currentRoom->hasMonster) {
                printf("⚠️  Er is een monster in deze kamer! Je moet eerst het monster verslaan.\n");
            } else {
                printf("🏆 Je hebt de schat gevonden! Gefeliciteerd!\n");
                break;
            }
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

        if (player.currentRoom->hasTreasure) {
            printf("🏆 Je hebt de schat gevonden! Gefeliciteerd!\n");
            break;
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
                player.currentRoom->monster->hp = baseMonsterHP * (1.1f) * (player.currentRoom->id + 1);
                player.currentRoom->monster->attack = baseMonsterAttack * (1.1f) * (player.currentRoom->id + 1);
                player.currentRoom->monster->defense = baseMonsterDefense * (1.1f) * (player.currentRoom->id + 1);
                player.currentRoom->monster->speed = baseMonsterSpeed * (1.1f) * (player.currentRoom->id + 1);
                player.currentRoom->monster->xp = baseMonsterXP * (player.currentRoom->id + 1);
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
    if (!rooms) {
        printf("Failed to allocate memory for rooms.\n");
        exit(1);
    }

    // Initialize the first monster with base stats
    rooms[0].id = 0;
    rooms[0].hasMonster = rand() % 2;
    rooms[0].hasItem = rand() % 2;
    rooms[0].hasTreasure = 0;
    for (int j = 0; j < 4; j++) {
        rooms[0].connections[j] = NULL;
    }

    rooms[0].monster = malloc(sizeof(Monster));
    if (!rooms[0].monster) {
        printf("Failed to allocate memory for monster.\n");
        exit(1);
    }
    rooms[0].monster->hp = baseMonsterHP;
    rooms[0].monster->attack = baseMonsterAttack;
    rooms[0].monster->defense = baseMonsterDefense;
    rooms[0].monster->speed = baseMonsterSpeed;
    rooms[0].monster->xp = baseMonsterXP;

    // Ensure monster deals at least 1 damage
    if (rooms[0].monster->attack < 1) {
        rooms[0].monster->attack = 1;
    }

    // Initialize subsequent monsters with increasing stats based on room ID
    for (int i = 1; i < numRooms; i++) {
        rooms[i].id = i;
        rooms[i].hasMonster = rand() % 2;
        rooms[i].hasItem = rand() % 2;
        rooms[i].hasTreasure = 0;
        for (int j = 0; j < 4; j++) {
            rooms[i].connections[j] = NULL;
        }

        rooms[i].monster = malloc(sizeof(Monster));
        if (!rooms[i].monster) {
            printf("Failed to allocate memory for monster.\n");
            exit(1);
        }

        // Scale the stats based on the room ID
        rooms[i].monster->hp = baseMonsterHP + (rooms[i].monster->hp  * 0.5);
        rooms[i].monster->attack = baseMonsterAttack + (rooms[i].monster->attack * 0.5);
        rooms[i].monster->defense = baseMonsterDefense + (rooms[i].monster->defense * 0.5);
        rooms[i].monster->speed = baseMonsterSpeed + (rooms[i].monster->speed * 0.5);
        rooms[i].monster->xp = baseMonsterXP + (rooms[i].monster->xp * 0.5);

        // Ensure monster deals at least 1 damage
        if (rooms[i].monster->attack < 1) {
            rooms[i].monster->attack = 1;
        }
    }

    // Place the chest in a room with ID >= 30
    for (int i = 30; i < numRooms; i++) {
        if (rand() % 2) {
            rooms[i].hasTreasure = 1;
            break;
        }
    }

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
    if (room->hasMonster) {
        printf("👹 Er is een monster in deze kamer! HP: %d, Attack: %d, Defense: %d, Speed: %d, XP: %d\n",
               room->monster->hp, room->monster->attack, room->monster->defense, room->monster->speed, room->monster->xp);
    }
    if (room->hasItem) {
        printf("✨ Je ziet een item liggen.\n");
    }
    if (room->hasTreasure) {
        printf("💰 De schat ligt hier!\n");
    }
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
    printf("👤 Speler - HP: %d | Damage: %d | Defense: %d | Speed: %d | Level: %d | Experience: %d/%d\n",
           player->hp, player->damage, player->defense, player->speed, player->level, player->experience, player->expToNextLevel);
    printf("👹 Monster - HP: %d | Attack: %d | Defense: %d | Speed: %d | XP: %d\n",
           player->currentRoom->monster->hp, player->currentRoom->monster->attack, player->currentRoom->monster->defense, player->currentRoom->monster->speed, player->currentRoom->monster->xp);

    while (player->currentRoom->monster->hp > 0 && player->hp > 0) {
        printf("\n----------------------------------\n"); // Separator line for each turn

        // Monster attack
        int dodgeChance = (player->speed - player->currentRoom->monster->speed) * 10;
        if (dodgeChance < 0) dodgeChance = 0; // Ensure dodge chance is not negative
        if (rand() % 100 > dodgeChance) {
            int damage = player->currentRoom->monster->attack - player->defense;
            if (damage < 1) damage = 1; // Ensure at least 1 damage
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
        int monsterDodgeChance = (player->currentRoom->monster->speed - player->speed) * 10;
        if (monsterDodgeChance < 0) monsterDodgeChance = 0; // Ensure dodge chance is not negative
        if (rand() % 100 > monsterDodgeChance) {
            int monsterDamage = player->damage - player->currentRoom->monster->defense;
            if (monsterDamage < 1) monsterDamage = 1; // Ensure at least 1 damage
            player->currentRoom->monster->hp -= monsterDamage;
            printf("👊 Jij valt aan! Monster heeft nu %d HP\n", player->currentRoom->monster->hp);
        } else {
            printf("💨 Monster ontwijkt je aanval! Monster HP: %d\n", player->currentRoom->monster->hp);
        }

        // Check if monster is still alive
        if (player->currentRoom->monster->hp <= 0) {
            printf("✅ Monster verslagen!\n");
            printf("🎉 Je krijgt %d ervaringpunten!\n", player->currentRoom->monster->xp);
            player->experience += player->currentRoom->monster->xp;

            // Increase player stats by 3 for each defeated monster
            player->hp += 3;
            player->damage += 3;
            player->defense += 3;
            player->speed += 3;
            printf("🌟 Je krijgt +3 in alle stats! HP: %d, Damage: %d, Defense: %d, Speed: %d\n",
                   player->hp, player->damage, player->defense, player->speed);

            // Check if player has enough experience to level up
            while (player->experience >= player->expToNextLevel) {
                levelUp(player);
            }

            break;
        }
    }
}

void getItem(Player* player) {
    int itemType = rand() % 4; // 0: Health, 1: Damage, 2: Defense, 3: Speed
    switch (itemType) {
        case 0:
            player->hp += 20;
            printf("❤️ Je vond een genezend item! (+20 HP)\n");
            break;
        case 1:
            player->damage += 5;
            printf("💪 Je vond een krachtitem! (+5 schade)\n");
            break;
        case 2:
            player->defense += 5;
            printf("🛡️ Je vond een verdedigingsitem! (+5 verdediging)\n");
            break;
        case 3:
            player->speed += 5;
            printf("💨 Je vond een snelheidsitem! (+5 snelheid)\n");
            break;
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
    // Increase player stats
    player->level++;
    player->hp += 10;          // Increase HP by 10
    player->damage += 5;       // Increase damage by 5
    player->defense += 5;      // Increase defense by 5
    player->speed += 5;        // Increase speed by 5

    // Calculate leftover experience points
    int leftoverExp = player->experience - player->expToNextLevel;
    player->experience = leftoverExp;

    // Increase the experience needed for the next level by 10
    player->expToNextLevel += 10;

    // Inform the player
    printf("🌟 Je bent gelevelupt! Nieuwe level: %d\n", player->level);
    printf("👤 Je nieuwe stats zijn: HP: %d, Damage: %d, Defense: %d, Speed: %d\n",
           player->hp, player->damage, player->defense, player->speed);
    printf("🎉 Je hebt nu %d ervaringpunten en %d ervaringpunten nodig om naar level %d te gaan.\n",
           player->experience, player->expToNextLevel, player->level + 1);
}

void displayPlayerStats(Player* player) {
    printf("\n--- Speler Status ---\n");
    printf("👤 Speler - HP: %d | Damage: %d | Defense: %d | Speed: %d | Level: %d | Experience: %d/%d\n",
           player->hp, player->damage, player->defense, player->speed, player->level, player->experience, player->expToNextLevel);
}