#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

void pack(char* archive_filename, char* src_directory) {
    DIR* dir;
    struct dirent *ent;
    char filepath[1024];
    FILE *archive_file = fopen(archive_filename, "w");
    FILE *current_file;
    char* buffer = (char*)malloc(sizeof(char)*1024*1024);
    size_t n;
    struct stat st;

    // check the directory is exists 
    if ((dir = opendir(src_directory)) != NULL){
        // read inside the directory  
        while ((ent = readdir(dir)) != NULL){
            // if read object is directory, just pass
            if (ent->d_type == DT_DIR) {
                continue;
            }
            // get path of the reading file in the src_directory
            snprintf(filepath, sizeof(filepath), "%s/%s", src_directory, ent->d_name);
            
            //get size of file
            if (stat(filepath, &st) != 0) {
                printf("Failed to get size of file: %s\n", filepath);
                continue;
            }

            // Open the current file
            current_file = fopen(filepath, "r");
            if (current_file == NULL) {
                printf("Failed to open file: %s\n", filepath);
                continue;
            }

            //write the file header. 
            fprintf(archive_file, "%s %zu\n", ent->d_name, st.st_size);

            //write the file context to archeive file
            while ((n = fread(buffer, 1, sizeof(buffer), current_file)) > 0) {
                fwrite(buffer, 1, n, archive_file);
            }
            fclose(current_file);
        }
        closedir(dir);
        printf("successfully pack files\n");
    }
    else{
        printf("Failed to open directory: %s\n", src_directory);
    }
    fclose(archive_file);
    free(buffer);
}

void unpack(char* archive_filename, char* dest_directory) {
    // archive file is exists?
    FILE *archive_file = fopen(archive_filename, "r");
    if(archive_file == NULL){
        printf("Error: Archive file %s does not exist\n", archive_filename);
        return;
    }

    FILE *current_file;
    char line[1024];
    char filename[1024];
    char filepath[1024];
    size_t filesize;
    char* buffer = (char*)malloc(sizeof(char)*1024*1024);
    size_t n;
    struct stat st = {0};

    // does des directory exists? if not, makedir.
    if (stat(dest_directory, &st) == -1) {
        mkdir(dest_directory,0700);
    }

    // Read the archive file
    while (fgets(line, sizeof(line), archive_file) != NULL) {
        sscanf(line, "%s %zu", filename, &filesize);
        // full path of the writing file
        snprintf(filepath, sizeof(filepath), "%s/%s", dest_directory, filename);
        // Open the file
        current_file = fopen(filepath, "w");
        if (current_file == NULL) {
            printf("Failed to open file: %s\n", filepath);
            continue;
        }
        // Read the current file's contents from the archive file and write them to the current file
        n = fread(buffer, 1, filesize, archive_file);    
        fwrite(buffer, 1, n, current_file);
        fflush(current_file);
        fclose(current_file);
    }
    printf("successfully unpacked\n");
    fclose(archive_file);
    free(buffer);
}

void add(char* archive_filename, char* target_filename) {
    //if add file name is already in archive file, error.
    FILE *archive_file_0 = fopen(archive_filename, "r");
    char line[1024];
    char filename[1024];
    size_t filesize;
    // Check if a file with the same name already exists in the archive
    while (fgets(line, sizeof(line), archive_file_0) != NULL) {
        sscanf(line, "%s %zu", filename, &filesize);
        if (strcmp(filename, target_filename) == 0) {
            printf("Error: File %s already exists in the archive\n", target_filename);
            fclose(archive_file_0);
            return;
        }
        // Skip the file's contents
        fseek(archive_file_0, filesize, SEEK_CUR);
    }
    fclose(archive_file_0);

    // add file to archive file.
    FILE *archive_file = fopen(archive_filename, "a");
    FILE *target_file;
    char* buffer = (char*)malloc(sizeof(char)*1024*1024);
    size_t n;
    struct stat st;

    // Get the size of the target file
    if (stat(target_filename, &st) != 0) {
        printf("Failed to get size of file: %s\n", target_filename);
        return;
    }

    // Open the target file
    target_file = fopen(target_filename, "r");
    if (target_file == NULL) {
        printf("Failed to open file: %s\n", target_filename);
        return;
    }

    // Write the target file's metadata to the archive file
    fprintf(archive_file, "%s %zu\n", target_filename, st.st_size);

    // read and store the target file data to buffer
    n = fread(buffer, 1, st.st_size, target_file);
    // data from buffer to archieve file
    fwrite(buffer, 1, n, archive_file);
    fclose(target_file);
    fclose(archive_file);
    free(buffer);
    printf("successfully add file\n");
}

void del(char* archive_filename, char* target_filename) {
    FILE *archive_file_0 = fopen(archive_filename, "r");
    char line_[1024];
    char filename_[1024];
    size_t filesize_;
    int cnt = 0;
    // Check if a target file exists in the archive
    while (fgets(line_, sizeof(line_), archive_file_0) != NULL) {
        sscanf(line_, "%s %zu", filename_, &filesize_);
        if (strcmp(filename_, target_filename) == 0) {
            cnt = 1;
        }
        // Skip the file's contents
        fseek(archive_file_0, filesize_, SEEK_CUR);
    }
    fclose(archive_file_0);
    //if target name doesn't exist in archieve file, return error
    if(cnt == 0){
        printf("wrong target filename: %s\n",target_filename);
        return;
    }

    //delete 
    FILE *old_archive_file = fopen(archive_filename, "r");
    FILE *new_archive_file = fopen("new_archive", "w");
    char line[1024];
    char filename[1024];
    char* buffer = (char*)malloc(sizeof(char)*1024*1024);
    size_t filesize;
    size_t n;

    // Read the old archive file
    while (fgets(line, sizeof(line), old_archive_file) != NULL) {
        sscanf(line, "%s %zu", filename, &filesize);
        // If this is the file to be deleted, skip it
        if (strcmp(filename, target_filename) == 0) {
            fseek(old_archive_file, filesize, SEEK_CUR);
            continue;
        }
        // Write the file's metadata to the new archive file
        fprintf(new_archive_file, "%s %zu\n", filename, filesize);

        // Read the file's contents from the old archive file and write them to the new archive file
        while (filesize > 0 && (n = fread(buffer, 1, sizeof(buffer) < filesize ? sizeof(buffer) : filesize, old_archive_file)) > 0) {
            fwrite(buffer, 1, n, new_archive_file);
            filesize -= n;
        }
    }

    fclose(old_archive_file);
    fclose(new_archive_file);
    free(buffer);
    // Replace the old archive file with the new one
    remove(archive_filename);
    rename("new_archive", archive_filename);
    printf("successfully delete the file from archive file\n");
}

void list(char* archive_filename) {
    FILE *archive_file = fopen(archive_filename, "r");
    char line[1024];
    char filename[1024];
    size_t filesize;
    int file_count = 0;

    // Read the archive file
    while (fgets(line, sizeof(line), archive_file) != NULL) {
        sscanf(line, "%s %zu", filename, &filesize);
        printf("File: %s, Size: %zu bytes\n", filename, filesize);
        file_count++;
        // Skip the file's contents
        fseek(archive_file, filesize, SEEK_CUR);
    }

    printf("Total files: %d\n", file_count);

    fclose(archive_file);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Not enough arguments\n");
        return 1;
    }
    char* command = argv[1];
    if (strcmp(command, "pack") == 0) {
        pack(argv[2], argv[3]);
    } else if (strcmp(command, "unpack") == 0) {
        unpack(argv[2], argv[3]);
    } else if (strcmp(command, "add") == 0) {
        add(argv[2], argv[3]);
    } else if (strcmp(command, "del") == 0) {
        del(argv[2], argv[3]);
    } else if (strcmp(command, "list") == 0) {
        list(argv[2]);
    } else {
        printf("Invalid command\n");
        return 1;
    }
    return 0;
}
