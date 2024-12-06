#include "fileops.h"

int File_Ops::do_copyin(const char *filename, int inumber, INE5412_FS *fs) {
    FILE *file;
    int offset = 0, result, actual;
    char buffer[16384];

    file = fopen(filename, "r");
    if (!file) {
        cout << "couldn't open " << filename << "\n";
        return 0;
    }

    while (1) {
        result = fread(buffer, 1, sizeof(buffer), file);
        if (result <= 0) break;
        if (result > 0) {
            actual = fs->fs_write(inumber, buffer, result, offset);
            if (actual == 0) return 0;
            if (actual < 0) {
                cout << "ERROR: fs_write return invalid result " << actual << "\n";
                break;
            }
            offset += actual;
            if (actual != result) {
                cout << "WARNING: fs_write only wrote " << actual << " bytes, not " << result << " bytes\n";
                break;
            }
        }
    }

    cout << offset << " bytes copied\n";

    fclose(file);

    return 1;
}

int File_Ops::do_copyout(int inumber, const char *filename, INE5412_FS *fs) {
    FILE *file;
    int offset = 0, result;
    char buffer[16384];

    file = fopen(filename, "w");
    if (!file) {
        cout << "couldn't open " << filename << "\n";
        return 0;
    }

    while (1) {
        result = fs->fs_read(inumber, buffer, sizeof(buffer), offset);
        if (result <= 0) break;
        fwrite(buffer, 1, result, file);
        offset += result;
    }

    cout << offset << " bytes copied\n";

    fclose(file);
    return 1;
}

stringstream File_Ops::cat(int inumber, INE5412_FS *fs) {
    int offset = 0, result;
    char buffer[16384];

    stringstream out;

    while (1) {
        result = fs->fs_read(inumber, buffer, sizeof(buffer), offset);
        if (result <= 0) break;
        offset += result;
        out << buffer;
    }

    cout << offset << " bytes copied\n";

    return out;
}