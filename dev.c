#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>    // For _getch() and kbhit()
#include <direct.h>   // For _mkdir()
#include <windows.h>  // For system(), Sleep(), GetSystemTime(), GetFileAttributes(), etc.

#define RESET_COLOR "\033[0m"
#define HIGHLIGHT_COLOR "\033[7m"
#define ARROW_UP 72
#define ARROW_DOWN 80
#define ENTER 13
#define ESC 27

// Define the editor to be used.
#define EDITOR "zed"

typedef struct ProjectNode {
    int language;     // 1 = Python, 2 = JavaScript (Node.js), 3 = Web (Vite)
    int isServer;     // Only applicable for Python and JavaScript projects.
    int isTemporary;  // 0 = Permanent, 1 = Temporary
    char projectName[256];
    char directory[512];
    char filename[64]; // Not used for Vite projects.
    struct ProjectNode* next;
} ProjectNode;

// Global flag for Ctrl+C detection
volatile int keepRunning = 1;

// Windows control handler for Ctrl+C (CTRL_C_EVENT)
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT) {
        printf("\nCtrl+C detected, exiting...\n");
        keepRunning = 0;
        return TRUE;
    }
    return FALSE;
}

void clearScreen();
void displayMenu(const char** options, int numOptions, int selectedOption);
int getArrowKeySelection(const char** options, int numOptions);
ProjectNode* createProjectNode();
void configureProject(ProjectNode* project);
void createFiles(ProjectNode* project);
void executeCommands(ProjectNode* project);
void freeNode(ProjectNode* project);
char* safeGets(char* buffer, size_t bufferSize);
int runAgain();

int main() {
    // Set up Ctrl+C handler using Windows API
    if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE)) {
        fprintf(stderr, "Error: Could not set control handler\n");
        return 1;
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    int continueRunning = 1;

    do {
        clearScreen();
        printf("Project Creator Tool\n");
        printf("====================\n\n");
        printf("Press Ctrl+C at any time to quit\n\n");

        ProjectNode* project = createProjectNode();
        if (project == NULL) {
            printf("Failed to allocate memory for project. Exiting...\n");
            return 1;
        }

        configureProject(project);
        if (!keepRunning) {
            freeNode(project);
            break;
        }

        createFiles(project);
        if (!keepRunning) {
            freeNode(project);
            break;
        }

        executeCommands(project);
        if (!keepRunning) {
            freeNode(project);
            break;
        }

        freeNode(project);

        if (!keepRunning) break;

        printf("\nProject creation completed!\n");
        continueRunning = runAgain();

    } while (continueRunning && keepRunning);

    printf("\nThank you for using Project Creator Tool. Goodbye!\n");
    printf("Press any key to exit...\n");
    _getch();

    return 0;
}

char* safeGets(char* buffer, size_t bufferSize) {
    if (!keepRunning) return NULL;
    if (fgets(buffer, (int)bufferSize, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        return buffer;
    }
    return NULL;
}

void clearScreen() {
    if (!keepRunning) return;
    system("cls");
}

void displayMenu(const char** options, int numOptions, int selectedOption) {
    if (!keepRunning) return;
    printf("\n");
    for (int i = 0; i < numOptions; i++) {
        if (i == selectedOption) {
            printf("%s> %s%s\n", HIGHLIGHT_COLOR, options[i], RESET_COLOR);
        } else {
            printf("  %s\n", options[i]);
        }
    }
    printf("\nUse arrow keys to navigate and Enter to select\n");
}

int getArrowKeySelection(const char** options, int numOptions) {
    if (!keepRunning) return -1;
    int selectedOption = 0;
    int key;

    displayMenu(options, numOptions, selectedOption);

    // Poll with kbhit() so that Ctrl+C can break out.
    while (keepRunning) {
        if (kbhit()) {
            key = _getch();
            if (key == 0 || key == 224) {
                key = _getch();
                if (key == ARROW_UP) {
                    selectedOption = (selectedOption > 0) ? selectedOption - 1 : numOptions - 1;
                } else if (key == ARROW_DOWN) {
                    selectedOption = (selectedOption < numOptions - 1) ? selectedOption + 1 : 0;
                }
                clearScreen();
                printf("Project Creator Tool\n");
                printf("====================\n\n");
                displayMenu(options, numOptions, selectedOption);
            }
            else if (key == ENTER) {
                return selectedOption;
            }
            else if (key == ESC) {
                printf("\nCancelled. Exiting...\n");
                exit(0);
            }
        } else {
            Sleep(50);
        }
    }
    return -1; // Return -1 if interrupted by Ctrl+C
}

ProjectNode* createProjectNode() {
    if (!keepRunning) return NULL;
    ProjectNode* newNode = (ProjectNode*)malloc(sizeof(ProjectNode));
    if (newNode != NULL) {
        newNode->language = 0;
        newNode->isServer = 0;
        newNode->isTemporary = 0;
        strcpy(newNode->projectName, "");
        strcpy(newNode->directory, "");
        strcpy(newNode->filename, "main");
        newNode->next = NULL;
    }
    return newNode;
}

void configureProject(ProjectNode* project) {
    if (!keepRunning) return;
    // Add "Web (Vite)" as a third option.
    const char* languageOptions[] = {"Python", "JavaScript (Node.js)", "Web (Vite)"};
    printf("Choose a project type:\n");
    int languageChoice = getArrowKeySelection(languageOptions, 3);
    if (!keepRunning || languageChoice == -1) return;
    project->language = languageChoice + 1; // 1,2,3

    clearScreen();
    printf("Project Creator Tool\n");
    printf("====================\n\n");
    printf("Selected project type: %s\n\n", languageOptions[languageChoice]);

    // For Python and JavaScript projects, ask if a server should be created.
    if (project->language != 3) {
        const char* serverOptions[] = {"No server", "Create a server"};
        printf("Do you want to create a server?\n");
        int serverChoice = getArrowKeySelection(serverOptions, 2);
        if (!keepRunning || serverChoice == -1) return;
        project->isServer = serverChoice;
    } else {
        // For Vite projects, we always run the dev server.
        project->isServer = 1;
        // For Vite, the filename field is not used.
        strcpy(project->filename, "");
    }

    // Ask if the project is temporary or permanent (applies to all types)
    const char* tempOptions[] = {"Permanent project", "Temporary project"};
    printf("Is this a temporary project?\n");
    int tempChoice = getArrowKeySelection(tempOptions, 2);
    if (!keepRunning || tempChoice == -1) return;
    project->isTemporary = tempChoice;

    clearScreen();
    printf("Project Creator Tool\n");
    printf("====================\n\n");
    printf("Project type: %s\n", languageOptions[languageChoice]);
    if (project->language != 3)
        printf("Server option: %s\n", (project->isServer) ? "Create a server" : "No server");
    printf("Project scope: %s\n\n", (project->isTemporary) ? "Temporary" : "Permanent");

    if (project->isTemporary == 1) {
        // Create a unique subfolder under TEMP.
        char* tempDir = getenv("TEMP");
        if (tempDir != NULL) {
            SYSTEMTIME st;
            GetSystemTime(&st);
            sprintf(project->directory, "%s\\Dev_%04d%02d%02d_%02d%02d%02d",
                    tempDir, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
            printf("Using temporary folder: %s\n", project->directory);
        } else {
            printf("Could not find TEMP directory. Exiting...\n");
            exit(1);
        }
    } else {
        char input[256];
        printf("Enter the project name: ");
        if (safeGets(input, sizeof(input)) == NULL) return;
        strcpy(project->projectName, input);
        sprintf(project->directory, "D:\\Computer-Science\\Projects\\All\\%s", project->projectName);
        printf("Creating project in: %s\n", project->directory);
    }

    // For Python/JavaScript projects, set a default filename.
    if (project->language == 1) { // Python
        strcpy(project->filename, (project->isServer) ? "app" : "main");
    } else if (project->language == 2) { // JavaScript (Node.js)
        strcpy(project->filename, (project->isServer) ? "index" : "main");
    }
}

void createFiles(ProjectNode* project) {
    if (!keepRunning) return;

    // For Vite web projects, let create-vite create the project folder.
    if (project->language == 3) {
        char command[1024];
        // Instead of pre-creating the folder, let npm create it.
        // Extract parent directory and folder name from project->directory.
        char *lastSlash = strrchr(project->directory, '\\');
        if (lastSlash != NULL) {
            char folderName[256];
            strcpy(folderName, lastSlash + 1);
            size_t parentLength = lastSlash - project->directory;
            char parentDir[512];
            strncpy(parentDir, project->directory, parentLength);
            parentDir[parentLength] = '\0';
            // Change directory into the parent and run the Vite generator.
            sprintf(command, "cd /d \"%s\" && npm create vite@latest %s", parentDir, folderName);
            int ret = system(command);
            if (ret != 0) {
                printf("Vite project creation cancelled or encountered an error.\n");
                keepRunning = 0;
                return;
            }
            printf("Vite project created in %s\n", project->directory);
        } else {
            sprintf(command, "npm create vite@latest \"%s\"", project->directory);
            int ret = system(command);
            if (ret != 0) {
                printf("Vite project creation cancelled or encountered an error.\n");
                keepRunning = 0;
                return;
            }
            printf("Vite project created in %s\n", project->directory);
        }
        return;
    } else {
        // For Python and JavaScript projects, create the project directory.
        _mkdir(project->directory);
    }

    if (project->language == 1) { // Python
        char filePath[1024];
        sprintf(filePath, "%s\\%s.py", project->directory, project->filename);
        FILE* file = fopen(filePath, "w");
        if (file == NULL) {
            printf("Error: Could not create file at %s\n", filePath);
            exit(1);
        }
        if (project->isServer == 1) {
            fprintf(file, "from flask import Flask\n");
            fprintf(file, "app = Flask(__name__)\n\n");
            fprintf(file, "@app.route(\"/\")\n");
            fprintf(file, "def hello_world():\n");
            fprintf(file, "    return \"Hello, World\"\n\n");
            fprintf(file, "if __name__ == \"__main__\":\n");
            fprintf(file, "    app.run(debug=True)\n");
            printf("Flask server code created in %s\n", filePath);
        } else {
            fprintf(file, "print(\"Hello World\")\n");
            printf("Python project created in %s without server.\n", filePath);
        }
        fclose(file);
    }
    else if (project->language == 2) { // JavaScript (Node.js)
        char filePath[1024];
        char extension[] = ".js";
        sprintf(filePath, "%s\\%s%s", project->directory, project->filename, extension);
        FILE* file = fopen(filePath, "w");
        if (file == NULL) {
            printf("Error: Could not create file at %s\n", filePath);
            exit(1);
        }
        if (project->isServer == 1) {
            char packagePath[1024];
            sprintf(packagePath, "%s\\package.json", project->directory);
            FILE* packageFile = fopen(packagePath, "w");
            if (packageFile != NULL) {
                fprintf(packageFile, "{\n");
                fprintf(packageFile, "  \"name\": \"%s\",\n", project->isTemporary ? "temp-project" : project->projectName);
                fprintf(packageFile, "  \"version\": \"1.0.0\",\n");
                fprintf(packageFile, "  \"description\": \"\",\n");
                fprintf(packageFile, "  \"main\": \"index.js\",\n");
                fprintf(packageFile, "  \"scripts\": {\n");
                fprintf(packageFile, "    \"start\": \"nodemon index.js\"\n");
                fprintf(packageFile, "  },\n");
                fprintf(packageFile, "  \"dependencies\": {\n");
                fprintf(packageFile, "    \"express\": \"^4.17.1\"\n");
                fprintf(packageFile, "  }\n");
                fprintf(packageFile, "}\n");
                fclose(packageFile);
                printf("Created package.json in %s\n", project->directory);
            }
            fprintf(file, "const express = require('express');\n");
            fprintf(file, "const app = express();\n");
            fprintf(file, "const port = 3000;\n\n");
            fprintf(file, "app.get('/', (req, res) => {\n");
            fprintf(file, "  res.send('Hello World!');\n");
            fprintf(file, "});\n\n");
            fprintf(file, "app.listen(port, () => {\n");
            fprintf(file, "  console.log(`Server running at http://localhost:${port}`);\n");
            fprintf(file, "});\n");
            printf("Node.js server code created in %s\n", filePath);
        } else {
            fprintf(file, "console.log('Hello World');\n");
            printf("JavaScript project created in %s without server.\n", filePath);
        }
        fclose(file);
    }
}

void executeCommands(ProjectNode* project) {
    if (!keepRunning) return;
    char command[1024];
    char innerCommand[1024];

    // For Vite projects, first check if the folder exists.
    if (project->language == 3) {
        DWORD attrib = GetFileAttributes(project->directory);
        if (attrib == INVALID_FILE_ATTRIBUTES || !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
            printf("Vite project creation was cancelled; skipping launch commands.\n");
            return;
        }
        // Open the project folder in the defined editor.
        sprintf(command, "start \"\" " EDITOR " \"%s\"", project->directory);
        system(command);
        // Launch a terminal to run "npm install" and "npm run dev".
        sprintf(innerCommand, "cd /d \"%s\" && npm install && npm run dev", project->directory);
        sprintf(command, "start \"\" cmd /k \"%s\"", innerCommand);
        system(command);
        return;
    }

    // For Python/JavaScript projects, determine file extension.
    char extension[4];
    if (project->language == 1) {
        strcpy(extension, ".py");
    } else if (project->language == 2) {
        strcpy(extension, ".js");
    } else {
        return;
    }

    // Construct the full file path for the main file.
    char filePath[1024];
    sprintf(filePath, "%s\\%s%s", project->directory, project->filename, extension);

    // Open the project folder and main file in the defined editor.
    sprintf(command, "start \"\" " EDITOR " \"%s\" \"%s\"", project->directory, filePath);
    system(command);

    if (project->isServer == 1) {
        if (project->language == 1) { // Python (Flask)
            sprintf(innerCommand, "cd /d \"%s\" && python %s%s && pause", project->directory, project->filename, extension);
            sprintf(command, "start \"\" cmd /k \"%s\"", innerCommand);
            system(command);
            system("timeout /t 3 > nul");
            system("start \"\" http://127.0.0.1:5000");
        } else if (project->language == 2) { // JavaScript (Node.js)
            sprintf(innerCommand, "cd /d \"%s\" && npm install && nodemon %s%s", project->directory, project->filename, extension);
            sprintf(command, "start \"\" cmd /k \"%s\"", innerCommand);
            system(command);
            system("timeout /t 5 > nul");
            system("start \"\" http://localhost:3000");
        }
    } else {
        if (project->language == 1) { // Python non-server
            sprintf(innerCommand, "cd /d \"%s\" && python %s%s && pause", project->directory, project->filename, extension);
            sprintf(command, "start \"\" cmd /k \"%s\"", innerCommand);
            system(command);
        } else if (project->language == 2) { // JavaScript non-server
            sprintf(innerCommand, "cd /d \"%s\" && node %s%s && pause", project->directory, project->filename, extension);
            sprintf(command, "start \"\" cmd /k \"%s\"", innerCommand);
            system(command);
        }
    }
}

void freeNode(ProjectNode* project) {
    if (project != NULL) {
        free(project);
    }
}

int runAgain() {
    if (!keepRunning) return 0;
    const char* continueOptions[] = {"Create a new project", "Quit"};
    printf("\nWhat would you like to do next?\n");
    int choice = getArrowKeySelection(continueOptions, 2);
    if (!keepRunning || choice == -1) return 0;
    return (choice == 0) ? 1 : 0;
}
