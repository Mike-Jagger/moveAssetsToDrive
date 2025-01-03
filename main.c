#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

const char *imageExtensions[] = {".jpg", ".jpeg", ".png", ".bmp", ".gif", ".tiff"};
const char *videoExtensions[] = {".mp4", ".mkv", ".avi", ".mov", ".wmv"};

int is_extension_match(const char *filename, const char **extensions, int extCount) {
    for (int i = 0; i < extCount; i++) {
        if (strstr(filename, extensions[i])) {
            return 1;
        }
    }
    return 0;
}

void move_file(const char *source, const char *destinationFolder) {
    char destinationPath[2048];
    snprintf(destinationPath, sizeof(destinationPath), "%s/%s", destinationFolder, strrchr(source, '/') + 1);
    #ifdef _WIN32
        MoveFile(source, destinationPath);
    #else
        rename(source, destinationPath);
    #endif
}

void create_folder_if_not_exists(const char *folderPath) {
    struct stat st = {0};
    if (stat(folderPath, &st) == -1) {
        #ifdef _WIN32
            mkdir(folderPath);
        #else
            mkdir(folderPath, 0777);
        #endif
    }
}

void move_files_from_list(const char *folderPath, const char *fileName, const char *destinationFolderName) {
    char sourcePath[2048];
    char destinationFolder[2048];
    char file[1024];

    snprintf(destinationFolder, sizeof(destinationFolder), "%s/%s", folderPath, destinationFolderName);
    create_folder_if_not_exists(destinationFolder);

    FILE *fileList = fopen(fileName, "r");
    if (!fileList) {
        perror("Failed to open file list");
        return;
    }

    while (fgets(file, sizeof(file), fileList)) {
        file[strcspn(file, "\n")] = 0; // Remove newline character
        snprintf(sourcePath, sizeof(sourcePath), "%s/%s", folderPath, file);
        move_file(sourcePath, destinationFolder);
    }

    fclose(fileList);
}

void process_files(const char *folderPath, const char **extensions, int extCount, const char *outputFileName, long long sizeLimit) {
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;
    long long cumulativeSize = 0;
    char filePath[2048];

    FILE *outputFile = fopen(outputFileName, "w");
    if (!outputFile) {
        perror("Failed to open output file");
        return;
    }

    if ((dir = opendir(folderPath)) == NULL) {
        perror("Failed to open directory");
        fclose(outputFile);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        snprintf(filePath, sizeof(filePath), "%s/%s", folderPath, entry->d_name);

        if (stat(filePath, &fileStat) == 0 && S_ISREG(fileStat.st_mode) && is_extension_match(entry->d_name, extensions, extCount)) {
            if (cumulativeSize + fileStat.st_size <= sizeLimit) {
                fprintf(outputFile, "%s\n", entry->d_name);
                cumulativeSize += fileStat.st_size;
            }
        }
    }

    closedir(dir);
    fclose(outputFile);
}

long long parse_size_limit(const char *input) {
    char unit;
    long long size;
    if (sscanf(input, "%lld%c", &size, &unit) != 2) {
        fprintf(stderr, "Invalid size format. Use format <number>[K|M|G].\n");
        exit(EXIT_FAILURE);
    }

    switch (unit) {
        case 'K':
        case 'k':
            return size * 1024LL;
        case 'M':
        case 'm':
            return size * 1024LL * 1024LL;
        case 'G':
        case 'g':
            return size * 1024LL * 1024LL * 1024LL;
        default:
            fprintf(stderr, "Invalid unit. Use K, M, or G.\n");
            exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <folder_path> <image_size_limit> <video_size_limit>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *folderPath = argv[1];
    long long imageSizeLimit = parse_size_limit(argv[2]);
    long long videoSizeLimit = parse_size_limit(argv[3]);

    // Ensure output folder exists
    #ifdef _WIN32
        mkdir("./out");
    #else
        mkdir("./out", 0777);
    #endif

    // Process images
    process_files(folderPath, imageExtensions, (int)(sizeof(imageExtensions) / sizeof(imageExtensions[0])), "./out/imageNames.txt", imageSizeLimit);

    // Process videos
    process_files(folderPath, videoExtensions, (int)(sizeof(videoExtensions) / sizeof(videoExtensions[0])), "./out/videoNames.txt", videoSizeLimit);

    // Move identified files
    move_files_from_list(folderPath, "./out/imageNames.txt", "images");
    move_files_from_list(folderPath, "./out/videoNames.txt", "videos");

    printf("Processing and moving completed. Results saved in ./out folder.\n");

    return EXIT_SUCCESS;
}