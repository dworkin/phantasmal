int file_size(string path) {
    int      i;
    mixed  **dir;
    string  *comps, base;

    if (path == "/") {
        return -2;
    }
    comps = explode(path, "/");
    base = comps[sizeof(comps) - 1];
    dir = get_dir(path);
    i = sizeof(dir[0]);
    while (i--) {
        if (dir[0][i] == base) {
            return dir[1][i];
        }
    }
    return -1;
}
