#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    char name[12];
    char extension[4];
    int attribute;
    int first_cluster;
    int size;
    struct Node* next;
} Node;

typedef struct {
    int num_bytes_per_sector;
    int num_sectors_per_cluster;
    int num_reserved_sectors;
    int num_sectors_in_directory;
    int num_sectors_in_data;
    int num_sectors_in_free_space;
    int num_files;
    int first_free_cluster;
} BootRecord;

void createFileSystemImage() {
    // Create the file system image with root directory containing a subdirectory "/folder1" and a file "test.txt" with content "inside"

    // Initialize the boot record
    BootRecord bootRecord;
    bootRecord.num_bytes_per_sector = 512;
    bootRecord.num_sectors_per_cluster = 1;
    bootRecord.num_reserved_sectors = 1;
    bootRecord.num_sectors_in_directory = 1;
    bootRecord.num_sectors_in_data = 1000;
    bootRecord.num_sectors_in_free_space = bootRecord.num_sectors_in_data;
    bootRecord.num_files = 0;
    bootRecord.first_free_cluster = 2;

    // Create the root directory linked list
    Node* rootDirectory = NULL;

    // Add subdirectory "/folder1" to the root directory
    Node* subdirectory = (Node*)malloc(sizeof(Node));
    strcpy(subdirectory->name, "folder1");
    strcpy(subdirectory->extension, "");
    subdirectory->attribute = 0;
    subdirectory->first_cluster = 2;
    subdirectory->size = 0;
    subdirectory->next = NULL;

    rootDirectory = subdirectory;
    bootRecord.num_files++;

    // Add file "test.txt" to the root directory
    Node* file = (Node*)malloc(sizeof(Node));
    strcpy(file->name, "test");
    strcpy(file->extension, "txt");
    file->attribute = 1; // Set attribute to indicate it's a file
    file->first_cluster = 3;
    file->size = 6;
    file->next = NULL;

    subdirectory->next = file;
    bootRecord.num_files++;

    // Create the file system image file
    FILE* fileSystemImage = fopen("filesystem.img", "wb");

    // Write the boot record to the file
    fwrite(&bootRecord, sizeof(BootRecord), 1, fileSystemImage);

    // Write the root directory to the file
    Node* currentNode = rootDirectory;
    while (currentNode != NULL) {
        fwrite(currentNode, sizeof(Node), 1, fileSystemImage);
        currentNode = currentNode->next;
    }

    // Write the data section to the file
    char dataSection[bootRecord.num_bytes_per_sector * bootRecord.num_sectors_in_data]; // Assuming each sector is 512 bytes and there
    memset(dataSection, 0, sizeof(dataSection)); // Initialize the data section with zeros

    // Write the content "inside" to the file "test.txt"
    strcpy(&dataSection[bootRecord.num_bytes_per_sector * file->first_cluster], "inside");

    fwrite(dataSection, sizeof(char), sizeof(dataSection), fileSystemImage);

    fclose(fileSystemImage);

    printf("File system image created successfully.\n");
}

void importFileToCurrentDirectory(const char* fileName, Node** rootDirectory, int* numFiles, BootRecord* bootRecord) {
    // Open the file to import from the system
    FILE* fileToImport = fopen(fileName, "rb");
    if (fileToImport == NULL) {
        printf("Failed to open the file.\n");
        return;
    }

    // Get the file size
    fseek(fileToImport, 0, SEEK_END);
    int fileSize = ftell(fileToImport);
    fseek(fileToImport, 0, SEEK_SET);

    // Check if there is enough free space in the file system image
    if (fileSize > bootRecord->num_sectors_in_free_space * bootRecord->num_bytes_per_sector) {
        printf("Not enough free space in the file system image.\n");
        fclose(fileToImport);
        return;
    }

    // Find the first free cluster to store the imported file
    int firstFreeCluster = bootRecord->first_free_cluster;
    bootRecord->first_free_cluster++;

    // Update the boot record with the new first free cluster
    FILE* fileSystemImage = fopen("filesystem.img", "rb+");
    fwrite(bootRecord, sizeof(BootRecord), 1, fileSystemImage);

    // Write the imported file content to the file system image
    char* fileContent = (char*)malloc(fileSize);
    fread(fileContent, sizeof(char), fileSize, fileToImport);
    fseek(fileSystemImage, firstFreeCluster * bootRecord->num_bytes_per_sector, SEEK_SET);
    fwrite(fileContent, sizeof(char), fileSize, fileSystemImage);

    // Create a new entry in the current directory for the imported file
    Node* newFile = (Node*)malloc(sizeof(Node));
    strcpy(newFile->name, fileName);
    strcpy(newFile->extension, "");
    newFile->attribute = 1; // Set attribute to indicate it's a file
    newFile->first_cluster = firstFreeCluster;
    newFile->size = fileSize;
    newFile->next = NULL;

    // Update the root directory with the new file entry
    if (*rootDirectory == NULL) {
        *rootDirectory = newFile;
    } else {
        Node* currentNode = *rootDirectory;
        while (currentNode->next != NULL) {
            currentNode = currentNode->next;
        }
        currentNode->next = newFile;
    }

    // Update the number of files in the boot record
    (*numFiles)++;

    // Write the updated root directory and boot record to the file system image
    fseek(fileSystemImage, sizeof(BootRecord), SEEK_SET);
    Node* currentNode = *rootDirectory;
    while (currentNode != NULL) {
        fwrite(currentNode, sizeof(Node), 1, fileSystemImage);
        currentNode = currentNode->next;
    }
    fseek(fileSystemImage, 0, SEEK_SET);
    fwrite(bootRecord, sizeof(BootRecord), 1, fileSystemImage);

    // Close the file
    fclose(fileSystemImage);
    fclose(fileToImport);

    printf("Espaço livre %d\n", bootRecord->num_sectors_in_free_space * bootRecord->num_bytes_per_sector);

    bootRecord->num_sectors_in_free_space -= fileSize/bootRecord->num_bytes_per_sector;

    printf("Espaço livre apos diminuir %d\n", bootRecord->num_sectors_in_free_space * bootRecord->num_bytes_per_sector);


    printf("File imported successfully.\n");
}

void createSubdirectory(Node** currentDirectory, BootRecord* bootRecord) {
    char subdirectoryName[12];
    printf("Enter the name of the subdirectory: ");
    scanf("%s", subdirectoryName);

    // Create a new subdirectory node
    Node* newSubdirectory = (Node*)malloc(sizeof(Node));
    strcpy(newSubdirectory->name, subdirectoryName);
    strcpy(newSubdirectory->extension, "");
    newSubdirectory->attribute = 0; // Set attribute to indicate it's a subdirectory
    newSubdirectory->first_cluster = bootRecord->first_free_cluster;
    newSubdirectory->size = 0;
    newSubdirectory->next = NULL;

    // Update theroot directory or current directory (depending on the implementation) with the new subdirectory entry
    if (*currentDirectory == NULL) {
        *currentDirectory = newSubdirectory;
    } else {
        Node* currentNode = *currentDirectory;
        while (currentNode->next != NULL) {
            currentNode = currentNode->next;
        }
        currentNode->next = newSubdirectory;
    }

    // Update the boot record with the new first free cluster
    bootRecord->first_free_cluster++;

    // Update the number of files in the boot record
    bootRecord->num_files++;

    // Update the file system image with the new subdirectory entry and updated boot record
    FILE* fileSystemImage = fopen("filesystem.img", "rb+");
    fseek(fileSystemImage, sizeof(BootRecord), SEEK_SET);
    Node* currentNode = *currentDirectory;
    while (currentNode != NULL) {
        fwrite(currentNode, sizeof(Node), 1, fileSystemImage);
        currentNode = currentNode->next;
    }
    fseek(fileSystemImage, 0, SEEK_SET);
    fwrite(bootRecord, sizeof(BootRecord), 1, fileSystemImage);

    // Close the file
    fclose(fileSystemImage);

    printf("Subdirectory created successfully.\n");
}

void navigate() {
    // List all content of root indexed and allow the user to navigate into subdirectories

    // Read the file system image file
    FILE* fileSystemImage = fopen("filesystem.img", "rb+");

    // Read the boot record from the file
    BootRecord bootRecord;
    fread(&bootRecord, sizeof(BootRecord), 1, fileSystemImage);

    // Read the root directory from the file
    Node* rootDirectory = NULL;
    Node* currentNode = NULL;

    for (int i = 0; i < bootRecord.num_files; i++) {
        Node* node = (Node*)malloc(sizeof(Node));
        fread(node, sizeof(Node), 1, fileSystemImage);
        node->next = NULL;

        if (rootDirectory == NULL) {
            rootDirectory = node;
            currentNode = rootDirectory;
        } else {
            currentNode->next = node;
            currentNode = currentNode->next;
        }
    }

    // Close the file
    fclose(fileSystemImage);

    // Display the contents of the root directory
    printf("Root Directory:\n");
    currentNode = rootDirectory;
    int itemCount = 0;
    while (currentNode != NULL) {
        printf("%d - %s.%s\n", itemCount + 1, currentNode->name, currentNode->extension);
        currentNode = currentNode->next;
        itemCount++;
    }
    printf("%d - Import a file to this directory\n", itemCount + 1);
    printf("%d - Create a new subdirectory\n", itemCount + 2);

    // Prompt the user for input
    int option;
    printf("Enter the number corresponding to a subdirectory or file to navigate into: ");
    scanf("%d", &option);

    // Check if the selected option is valid
    if (option >= 1 && option <= itemCount) {
        // Get the selected entry
        currentNode = rootDirectory;
        for (int i = 1; i < option; i++) {
            currentNode = currentNode->next;
        }

        // Check if the selected entry is a subdirectory
        if (currentNode->attribute == 0) {
            // Read the subdirectory from the file
            Node* subdirectory = NULL;
            Node* subdirectoryNode = NULL;
            FILE* fileSystemImage = fopen("filesystem.img", "rb+");
            fseek(fileSystemImage, currentNode->first_cluster * bootRecord.num_bytes_per_sector, SEEK_SET);

            for (int i = 0; i < bootRecord.num_files; i++) {
                Node* node = (Node*)malloc(sizeof(Node));
                fread(node, sizeof(Node), 1, fileSystemImage);
                node->next = NULL;

                if (subdirectory == NULL) {
                    subdirectory = node;
                    subdirectoryNode = subdirectory;
                } else {
                    subdirectoryNode->next = node;
                    subdirectoryNode = subdirectoryNode->next;
                }
            }

            // Close the file
            fclose(fileSystemImage);

            // Display the contents of the subdirectory
            printf("Subdirectory: %s\n", currentNode->name);
            subdirectoryNode = subdirectory;
            while (subdirectoryNode != NULL) {
                printf("%s.%s\n", subdirectoryNode->name, subdirectoryNode->extension);
                subdirectoryNode = subdirectoryNode->next;
            }

            // Free the allocated memory
            while (subdirectory != NULL) {
                subdirectoryNode = subdirectory;
                subdirectory = subdirectory->next;
                free(subdirectoryNode);
            }
            // Provide options to import a file or create a new subdirectory
            printf("%d - Import a file to this directory\n", itemCount + 1);
            printf("%d - Create a new subdirectory\n", itemCount + 2);

            // Prompt the user for additional input
            int subOption;
            printf("Enter the number corresponding to an option: ");
            scanf("%d", &subOption);

            if (subOption == itemCount + 1) {
                // Import a file from the system to the current directory
                char fileName[100];
                printf("Enter the name of the file to import (including extension): ");
                scanf("%s", fileName);

                importFileToCurrentDirectory(fileName, &rootDirectory, &bootRecord.num_files, &bootRecord);
            } else if (subOption == itemCount + 2) {
                // Create a new subdirectory in the current directory
                createSubdirectory(&rootDirectory, &bootRecord);
            } else {
                printf("Invalid option.\n");
            }
        } else {
            // Read the file content from the file
            char* fileContent = (char*)malloc(currentNode->size);
            FILE* fileSystemImage = fopen("filesystem.img", "rb+");
            fseek(fileSystemImage, currentNode->first_cluster * bootRecord.num_bytes_per_sector, SEEK_SET);
            fread(fileContent, sizeof(char), currentNode->size, fileSystemImage);

            // Close the file
            fclose(fileSystemImage);

            // Display the content of the file
            printf("File: %s.%s\n", currentNode->name, currentNode->extension);
            printf("Content: %s\n", fileContent);

            free(fileContent);
        }
    } else if (option == itemCount + 1) {
        // Import a file from the system to the current directory
        char fileName[100];
        printf("Enter the name of the file to import (including extension): ");
        scanf("%s", fileName);

        importFileToCurrentDirectory(fileName, &rootDirectory, &bootRecord.num_files, &bootRecord);
    } else if (option == itemCount + 2) {
        // Create a new subdirectory in the current directory
        createSubdirectory(&rootDirectory, &bootRecord);
    } else {
        printf("Invalid option.\n");
    }

    // Free the allocated memory
    while (rootDirectory != NULL) {
        currentNode = rootDirectory;
        rootDirectory = rootDirectory->next;
        free(currentNode);
    }
}

int main() {
    int option;
    printf("Options:\n");
    printf("1 - Create file system image\n");
    printf("2 - Navigate\n");
    printf("Enter your option: ");
    scanf("%d", &option);

    switch (option) {
        case 1:
            createFileSystemImage();
            break;
        case 2:
            navigate();
            break;
        default:
            printf("Invalid option\n");
            break;
    }

    return 0;
}