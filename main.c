#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define IMAGE_SIZE_LIMIT (15LL * 1024 * 1024 * 1024) // 15 GB
#define VIDEO_SIZE_LIMIT (20LL * 1024 * 1024 * 1024) // 20 GB

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

void process_files(const char *folderPath, const char **extensions, int extCount, const char *outputFileName, long long sizeLimit) {
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;
    long long cumulativeSize = 0;
    char filePath[1024];

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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <folder_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *folderPath = argv[1];

    // Ensure output folder exists
    #ifdef _WIN32
        mkdir("./out");
    #else
        mkdir("./out", 0777);
    #endif

    // Process images
    process_files(folderPath, imageExtensions, (int)(sizeof(imageExtensions) / sizeof(imageExtensions[0])), "./out/imageNames.txt", IMAGE_SIZE_LIMIT);

    // Process videos
    process_files(folderPath, videoExtensions, (int)(sizeof(videoExtensions) / sizeof(videoExtensions[0])), "./out/videoNames.txt", VIDEO_SIZE_LIMIT);

    printf("Processing completed. Results saved in ./out folder.\n");

    return EXIT_SUCCESS;
}
