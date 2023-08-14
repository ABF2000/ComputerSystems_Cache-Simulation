#include "memory.h"
#include "stdio.h"
#include <stdlib.h>
#include "string.h"
#include <math.h>
#include <stdint.h>


// Vi skal calloc'e plads, for at lave plads til vores cache
// og vi skal bruge memory.c/memory.h til at hvis værdien vi prøver at læse
// med f.eks. kommandoen: r 01 01, og den så fejler (at værdien ikke findes i cachen) 

typedef struct block {
    int *dataElement;
    int validBits;
    int tags;
} block_t;

typedef struct set {
    block_t* blockList;
    int *RLU;
    int RLUCounter;
} set_t;

typedef struct cache {
    set_t* setList;
} cache_t;




/*
Vi skal ihvertfald bruge de to øverste structs til at holde styr og kunne søge i/skrive til vores cache

Da vi skal kunne modtage flere forskellige operationer efter en konfiguration, skal vi lave et while-loop
som læser fra terminalen. Der skal gives følgende struktur, når vi ønsker at udfører en operation:
op addr data


INIT skal initialiserer en cache med nogle værdier
- dette skal først gøres, når man vil teste sin cache

READ skal ud fra addressen og dataen læse på den cache addresse

WRITE skal skrive dataen til addressen i cachen
*/


// Dette er bare en hjælpe funktion til at comparer den første token længere nede
int OperationChecker(char* operationString){
    if (strcmp(operationString, "i") == 0){
        return 0;
    }
    else if (strcmp(operationString, "r") == 0){
        return 0;
    }
    else if (strcmp(operationString, "w") == 0){
        return 0;
    } 
    else {
        return 1;
    }
}

int hex_to_decimal(const char *hex) {
    // Initialize result
    int decimal = 0;

    // Iterate over each hexadecimal digit
    while (*hex) {
        // Get the current digit
        char digit = *hex;

        // If the digit is a decimal digit (0-9), add its value to the result
        if (digit >= '0' && digit <= '9') {
            decimal = decimal * 16 + (digit - '0');
        }
        // If the digit is an uppercase or lowercase letter (A-F or a-f), add its value to the result
        else if (digit >= 'A' && digit <= 'F') {
            decimal = decimal * 16 + (digit - 'A' + 10);
        } else if (digit >= 'a' && digit <= 'f') {
            decimal = decimal * 16 + (digit - 'a' + 10);
        } else {
            // Invalid hexadecimal digit
            fprintf(stderr, "Error: Invalid hexadecimal digit '%c'\n", digit);
            exit(EXIT_FAILURE);
        }

        // Move to the next digit
        hex++;
    }

    // Return the result
    return decimal;
}

int AddressChecker(char* addressString){
    int addrAsDecimal = hex_to_decimal(addressString);
    if ((addrAsDecimal % 4) == 0){
        return 0;
    }
    else {
        return 1;
    }
}

// Denne funktion skal finde indexet forde n givne addresse, og enten
// returnere indexet eller -1 hvis den ikke findes i RLU-listen
int FindRLUIndex(int blockAddress, cache_t *cache, int setIndex){
    for (int i = 0; i < cache->setList[setIndex].RLUCounter; i++){
        if (blockAddress == cache->setList[setIndex].RLU[i]){
            return i;
        }
    }
    return -1;
}

// Efter indexet er fundet, så skal addressen enten tilføjes til slutningen af listen
// eller rykkes til slutningen af lysten
void ShiftAddressInRLU(int blockAddress, int addrIndex, cache_t *cache, int setIndex){
    if (addrIndex == -1){
        cache->setList[setIndex].RLU[cache->setList[setIndex].RLUCounter] = blockAddress;
        cache->setList[setIndex].RLUCounter += 1;
        return;
    }
    if (cache->setList[setIndex].RLU[cache->setList[setIndex].RLUCounter-1] == blockAddress){
        return;
    }
    else {
        int temp = cache->setList[setIndex].RLU[addrIndex];
        for (int i = addrIndex; i < cache->setList[setIndex].RLUCounter - 1; i++){
            cache->setList[setIndex].RLU[i] = cache->setList[setIndex].RLU[i + 1];
        }
        cache->setList[setIndex].RLU[cache->setList[setIndex].RLUCounter - 1] = temp;
        return;
    }
}

// Denne funktion er hoved-funktionen til at opdaterer vores RLU-liste
void UpdateRLU(int blockAddress, cache_t *cache, int setIndex){
    int RLUCount = cache->setList[setIndex].RLUCounter;
    if (RLUCount == 0){
        cache->setList[setIndex].RLU[0] = blockAddress;
        cache->setList[setIndex].RLUCounter += 1;
        return;
    }
    else {
        int addrIndex = FindRLUIndex(blockAddress, cache, setIndex);
        ShiftAddressInRLU(blockAddress, addrIndex, cache, setIndex);
    }
}

int GetOldestRLUAddress(cache_t *cache, int setIndex){
    return cache->setList[setIndex].RLU[0];
}

void PrintRLUList(cache_t *cache, int setIndex){
    printf(" lru-order: ");
    for (int i = 0; i < cache->setList[setIndex].RLUCounter; i++){
        printf("%i, ", cache->setList[setIndex].RLU[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]){
    if (argc == 4){
        int nWayCache = atoi(argv[1]);
        int setAmount = atoi(argv[2]);
        int blockSize = atoi(argv[3]);

        if (nWayCache > 16){
            printf("Maximum number of nWays exceeded\n");
            printf("Exiting...\n");
            exit(0);
        }
        if (setAmount % 2 != 0){
            printf("Set-amount has to be a power of 2\n");
            printf("Exiting...\n");
            exit(0);
        }
        if ((blockSize % 2 != 0) || (blockSize < 4)){
            printf("Block-size has to be a power of 2\n");
            printf("and must be at least 4\n");
            printf("Exiting...\n");
            exit(0);
        }
        
        cache_t mainCache;

        mainCache.setList = (set_t *) malloc(setAmount * sizeof(set_t));

        for (int i = 0; i < setAmount; i++){
            mainCache.setList[i].blockList = (block_t *) malloc(nWayCache * sizeof(block_t));
            mainCache.setList[i].RLU = (int *) malloc(nWayCache * sizeof(int));
            for (int j = 0; j < nWayCache; j++){
                mainCache.setList[i].blockList[j].validBits = 0;
                mainCache.setList[i].blockList[j].tags = j;
                mainCache.setList[i].blockList[j].dataElement = (int *) malloc(blockSize * sizeof(int));
                for (int k = 0; k < blockSize; k++){
                    mainCache.setList[i].blockList[j].dataElement[k] = 0;
                }
                mainCache.setList[i].RLUCounter = 0;
            }
        }

        struct memory *newMemory = memory_create();

        // Starter et while-loop her for at kunne modtage operationer
        while(1){
            // Her initializer jeg en string med den maximale plads, som EN kommando-linje kan have
            // og henter så input fra useren
            char inputString[8192];
            fgets(inputString, 8192, stdin);

            // HERFRA
            char* tokenString = strtok(inputString, " ");
            char* tokenizedString[3];
            int i = 0;

            while (tokenString != NULL){
                tokenizedString[i++] = tokenString;
                tokenString = strtok(NULL, " ");
            }
            // OG HERTIL, tokenizer jeg den input-string vi har fået af useren,
            // så vi kan bruge de individuelle argumenter hver for sig

            if (OperationChecker(tokenizedString[0]) == 1){
                printf("Invalid Operation\n");
            }

            int bitsTag = 32 - log2(setAmount) - log2(blockSize);
            int bitsSetIndex = log2(setAmount);
            int bitsBlockOffset = log2(blockSize);

            // Næste step her er at omregne en addreese til en bestemt block

            // en 32-bit addresse er opdelt i TAG | SET-INDEX | BLOCK-OFFSET

            // antallet af bits der bruges til BLOCK-OFFSET findes ved at tage log_2() af block-sizen

            // antallet af bits der bruges til SET-INDEX findes ved at tage log_2() af antallet af sets

            // antallet af bits der bruges til TAG findes ved bare at bruge de resterende bits i addressen

            

            // Når vi har tilføjet denne funktionalitet, så kan vi begynde at se på cache-misses, altså hvis 
            // den efterspugte værdi ikke findes i cachen, så skal den ned og hentes i memory (det er her memory.h/
            // memory.c kommer ind)

            if (strcmp(tokenizedString[0], "r") == 0){

                int data = 0;

                // Da vi har en hex-addresse som en string, så skal vi først converte den til en int
                int addrInDecimal = hex_to_decimal(tokenizedString[1]);

                // Og da vi så har en int, kan vi begynde at bruge bitshifting, samt bitmasking, for at finde
                // tagget, set-indexet og block-offsettet
                uint32_t mask = (1 << bitsSetIndex) - 1;
                int addrSetIndex = (addrInDecimal >> bitsBlockOffset) & mask;
                mask = (1 << bitsBlockOffset) - 1;
                int addrBlockOffset = addrInDecimal & mask;
                int addrTag = addrInDecimal >> (bitsSetIndex + bitsBlockOffset); 

                if (AddressChecker(tokenizedString[1]) != 0){
                    printf("Address not divisible by 4\n");
                    continue;
                }

                // Da vi er inde i det if-statement, hvor operationen er at læse, skal vi bare gemme den værdi
                // som vi finder
                if (mainCache.setList[addrSetIndex].blockList[addrTag].validBits == 1){

                    UpdateRLU(addrInDecimal, &mainCache, addrSetIndex);

                    data = mainCache.setList[addrSetIndex].blockList[addrTag].dataElement[addrBlockOffset];
                    printf("r %s %d HIT ", tokenizedString[1], data);
                    PrintRLUList(&mainCache, addrSetIndex);
                    continue;
                }
                else {

                    int dataInMemory = memory_rd_w(newMemory, addrInDecimal);

                    if (dataInMemory != 0){
                        for (int i = 0; i < nWayCache; i++){

                            if (mainCache.setList[addrSetIndex].blockList[i].dataElement[addrBlockOffset] == 0){
                                mainCache.setList[addrSetIndex].blockList[i].dataElement[addrBlockOffset] = dataInMemory;
                                mainCache.setList[addrSetIndex].blockList[i].validBits = 1;
                                UpdateRLU(addrInDecimal, &mainCache, addrSetIndex);
                                printf("r %s %d FILL ", tokenizedString[1], dataInMemory);
                                PrintRLUList(&mainCache, addrSetIndex);
                                break;
                            }
                        }
                        continue;
                    }
                    else {
                        printf("r %s (no data) MISS ", tokenizedString[1]);
                        PrintRLUList(&mainCache, addrSetIndex);
                        continue;
                    }
                }
            }

            if (strcmp(tokenizedString[0], "w") == 0){
                int data = atoi(tokenizedString[2]);

                // Da vi har en hex-addresse som en string, så skal vi først converte den til en int
                int addrInDecimal = hex_to_decimal(tokenizedString[1]);

                // Og da vi så har en int, kan vi begynde at bruge bitshifting, samt bitmasking, for at finde
                // tagget, set-indexet og block-offsettet
                uint32_t mask = (1 << bitsSetIndex) - 1;
                int addrSetIndex = (addrInDecimal >> bitsBlockOffset) & mask;
                mask = (1 << bitsBlockOffset) - 1;
                int addrBlockOffset = addrInDecimal & mask;
                int addrTag = addrInDecimal >> (bitsSetIndex + bitsBlockOffset); 

                // Her laver cachen en write-through, altså når den skriver til cachen, så skriver den også
                // til memory med det samme (ikke den bedste performance, men det virker)
                
                if (AddressChecker(tokenizedString[1]) != 0){
                    printf("w %s %s DISCARD ", tokenizedString[1], tokenizedString[2]);
                    PrintRLUList(&mainCache, addrSetIndex);
                    continue;
                }

                if (mainCache.setList[addrSetIndex].blockList[addrTag].dataElement[addrBlockOffset] == 0){
                    mainCache.setList[addrSetIndex].blockList[addrTag].dataElement[addrBlockOffset] = data;
                    mainCache.setList[addrSetIndex].blockList[addrTag].validBits = 1;
                    memory_wr_w(newMemory, addrInDecimal, data);
                    UpdateRLU(addrInDecimal, &mainCache, addrSetIndex);
                    printf("w %s %s FILL ", tokenizedString[1], tokenizedString[2]);
                    PrintRLUList(&mainCache, addrSetIndex);
                    continue;
                }

                // This should be replaced with an LRU-scheme, but that hasn't been implemented yet
                if (mainCache.setList[addrSetIndex].blockList[addrTag].dataElement[addrBlockOffset] != 0){
                    int replacementAddress = GetOldestRLUAddress(&mainCache, addrSetIndex);
                    int replacementIndex = FindRLUIndex(replacementAddress, &mainCache, addrSetIndex);

                    int oldData = mainCache.setList[addrSetIndex].blockList[replacementIndex].dataElement[addrBlockOffset];
                    memory_wr_w(newMemory, replacementAddress, oldData);

                    mainCache.setList[addrSetIndex].blockList[replacementIndex].dataElement[addrBlockOffset] = data;
                    UpdateRLU(replacementAddress, &mainCache, addrSetIndex);
                    printf("w %s %s EVICT ", tokenizedString[1], tokenizedString[2]);
                    PrintRLUList(&mainCache, addrSetIndex);
                }

            }

            if (strcmp(tokenizedString[0], "i") == 0){
                int data = atoi(tokenizedString[2]);

                if (AddressChecker(tokenizedString[1]) == 0){
                    int addrInDecimal = hex_to_decimal(tokenizedString[1]);
                    memory_wr_w(newMemory, addrInDecimal, data);
                    printf("i %s %s INIT\n", tokenizedString[1], tokenizedString[2]);
                    continue;
                }
                printf("Address not divisible by 4\n");
                continue;
            }

        }
    }
    else {
        printf("Incorrect amount of arguments\n");
        printf("Arguments should be given in the following order:\n");
        printf("number of way-associative cache\n");
        printf("number of sets\n");
        printf("block-size\n");
    }
}