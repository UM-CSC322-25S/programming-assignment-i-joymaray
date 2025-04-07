#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120     // Maximum number of boats the system can hold
#define MAX_NAME 128      // Maximum length of boat names

// Enum for different boat storage types
typedef enum {
    SLIP,
    LAND,
    TRAILOR,
    STORAGE
} BoatType;

// Union to store type-specific location data
typedef union {
    int slipNumber;
    char bayLetter;
    char licenseTag[16];
    int storageNumber;
} Location;

// Struct representing a boat's data
typedef struct {
    char name[MAX_NAME];
    float length;
    BoatType type;
    Location loc;
    float amountOwed;
} Boat;

// Global array of pointers to boats and a counter
Boat *marina[MAX_BOATS];
int boatCount = 0;

// Trims whitespace from the start and end of a string
char *trim(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;
    return str;
}

// Reads boats from a CSV file and populates the marina array
int readCSV(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (boatCount >= MAX_BOATS) break;

        Boat *b = malloc(sizeof(Boat));
        char typeStr[16];
        char extra[32];

        // Parse the line from CSV into boat fields
        sscanf(line, "%127[^,],%f,%15[^,],%31[^,],%f", b->name, &b->length, typeStr, extra, &b->amountOwed);

        // Normalize type string to lowercase
        for (char *p = typeStr; *p; p++) *p = tolower(*p);

        // Set type and extra info based on the type
        if (strcmp(typeStr, "slip") == 0) {
            b->type = SLIP;
            b->loc.slipNumber = atoi(extra);
        } else if (strcmp(typeStr, "land") == 0) {
            b->type = LAND;
            b->loc.bayLetter = extra[0];
        } else if (strcmp(typeStr, "trailor") == 0) {
            b->type = TRAILOR;
            strncpy(b->loc.licenseTag, extra, 15);
        } else if (strcmp(typeStr, "storage") == 0) {
            b->type = STORAGE;
            b->loc.storageNumber = atoi(extra);
        }

        marina[boatCount++] = b;
    }

    fclose(file);
    return 1;
}

// Saves current boat data to a CSV file
void saveCSV(char *filename) {
    FILE *file = fopen(filename, "w");

    for (int i = 0; i < boatCount; i++) {
        Boat *b = marina[i];
        char *typeStr = (b->type == SLIP) ? "slip" :
                        (b->type == LAND) ? "land" :
                        (b->type == TRAILOR) ? "trailor" :
                        "storage";

        fprintf(file, "%s,%.0f,%s,", b->name, b->length, typeStr);

        // Write the type-specific field
        if (b->type == SLIP)
            fprintf(file, "%d,", b->loc.slipNumber);
        else if (b->type == LAND)
            fprintf(file, "%c,", b->loc.bayLetter);
        else if (b->type == TRAILOR)
            fprintf(file, "%s,", b->loc.licenseTag);
        else if (b->type == STORAGE)
            fprintf(file, "%d,", b->loc.storageNumber);

        fprintf(file, "%.2f\n", b->amountOwed);
    }

    fclose(file);
}

// Helper function for sorting boats alphabetically by name (case-insensitive)
int compare_letters(const void *a, const void *b) {
    return strcasecmp((*(Boat **)a)->name, (*(Boat **)b)->name);
}

// Prints a sorted inventory of all boats
void printInventory() {
    qsort(marina, boatCount, sizeof(Boat *), compare_letters);

    for (int i = 0; i < boatCount; i++) {
        Boat *b = marina[i];
        printf("%-20s %4.0f' ", b->name, b->length);

        // Display the type-specific location info
        if (b->type == SLIP)
            printf("slip   # %2d", b->loc.slipNumber);
        else if (b->type == LAND)
            printf("land      %c", b->loc.bayLetter);
        else if (b->type == TRAILOR)
            printf("trailor %s", b->loc.licenseTag);
        else if (b->type == STORAGE)
            printf("storage # %2d", b->loc.storageNumber);

        printf("   Owes $%7.2f\n", b->amountOwed);
    }
}

// Adds a new boat from a single CSV-style input string
void addBoat(char *line) {
    if (boatCount >= MAX_BOATS) return;

    Boat *b = malloc(sizeof(Boat));
    char typeStr[16], extra[32];

    sscanf(line, "%127[^,],%f,%15[^,],%31[^,],%f", b->name, &b->length, typeStr, extra, &b->amountOwed);

    for (char *p = typeStr; *p; p++) *p = tolower(*p);

    if (strcmp(typeStr, "slip") == 0) {
        b->type = SLIP;
        b->loc.slipNumber = atoi(extra);
    } else if (strcmp(typeStr, "land") == 0) {
        b->type = LAND;
        b->loc.bayLetter = extra[0];
    } else if (strcmp(typeStr, "trailor") == 0) {
        b->type = TRAILOR;
        strncpy(b->loc.licenseTag, extra, 15);
    } else if (strcmp(typeStr, "storage") == 0) {
        b->type = STORAGE;
        b->loc.storageNumber = atoi(extra);
    }

    marina[boatCount++] = b;
}

// Finds the index of a boat by name (case-insensitive)
int findBoat(char *name) {
    for (int i = 0; i < boatCount; i++) {
        if (strcasecmp(name, marina[i]->name) == 0)
            return i;
    }
    return -1;
}

// Removes a boat by name
void removeBoat(char *name) {
    int idx = findBoat(name);
    if (idx == -1) {
        printf("No boat with that name\n");
        return;
    }

    free(marina[idx]);

    // Shift remaining boats left
    for (int i = idx; i < boatCount - 1; i++)
        marina[i] = marina[i + 1];

    boatCount--;
}

// Applies a payment to a boat, if the amount is valid
void makePayment(char *name, float amount) {
    int idx = findBoat(name);
    if (idx == -1) {
        printf("No boat with that name\n");
        return;
    }

    if (amount > marina[idx]->amountOwed) {
        printf("That is more than the amount owed, $%.2f\n", marina[idx]->amountOwed);
        return;
    }

    marina[idx]->amountOwed -= amount;
}

// Increases the amount owed for all boats based on their type
void newMonth() {
    for (int i = 0; i < boatCount; i++) {
        Boat *b = marina[i];
        float rate = 0;

        switch (b->type) {
            case SLIP: rate = 12.50f; break;
            case LAND: rate = 14.00f; break;
            case TRAILOR: rate = 25.00f; break;
            case STORAGE: rate = 11.20f; break;
        }

        b->amountOwed += rate * b->length;
    }
}

// Main program loop
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s BoatData.csv\n", argv[0]);
        return 1;
    }

    readCSV(argv[1]);

    printf("Welcome to the Boat Management System\n-------------------------------------\n");

    char option[8];

    while (1) {
        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        fgets(option, sizeof(option), stdin);
        char op = tolower(option[0]);

        if (op == 'i') {
            printInventory();
        } else if (op == 'a') {
            char line[256];
            printf("Please enter the boat data in CSV format                 : ");
            fgets(line, sizeof(line), stdin);
            addBoat(trim(line));
        } else if (op == 'r') {
            char name[MAX_NAME];
            printf("Please enter the boat name                               : ");
            fgets(name, sizeof(name), stdin);
            removeBoat(trim(name));
        } else if (op == 'p') {
            char name[MAX_NAME];
            float amt;
            printf("Please enter the boat name                               : ");
            fgets(name, sizeof(name), stdin);
            int idx = findBoat(trim(name));
            if (idx == -1) {
                printf("No boat with that name\n");
                continue;
            }
            printf("Please enter the amount to be paid                       : ");
            scanf("%f%*c", &amt); // Read float and consume newline
            makePayment(trim(name), amt);
        } else if (op == 'm') {
            newMonth();
        } else if (op == 'x') {
            break;
        } else {
            printf("Invalid option %c\n", option[0]);
        }
    }

    saveCSV(argv[1]);
    printf("\nExiting the Boat Management System\n");
    return 0;
}

