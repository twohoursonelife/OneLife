#ifndef DBSHRINK_H_INCLUDED
#define DBSHRINK_H_INCLUDED

int DB_open_modeSwitch(
    DB *db,
    const char *path,
    int mode,
    unsigned long hash_table_size,
    unsigned long key_size,
    unsigned long value_size);
    
int DB_open_naturalTileShrunk_both(
    DB *db,
    const char *path,
    int mode,
    unsigned long hash_table_size,
    unsigned long key_size,
    unsigned long value_size,
    DB *db_time,
    const char *path_time,
    int mode_time,
    unsigned long hash_table_size_time,
    unsigned long key_size_time,
    unsigned long value_size_time);
    
void loadDBShrinkSettings();
    
#endif