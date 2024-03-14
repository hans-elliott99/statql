
#include <stdio.h>
#include "sqlite/sqlite3.h"

/*
    connect to sqlite3 database
*/

sqlite3 *db;
int rc;
char errbuff[1001];


int main() {
    int ret = 0;
    printf("Connecting to test.db...\n");

    rc = sqlite3_open("test.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        ret = 1;
        goto out;
    }

    // sqlite3_exec(db, "SELECT * FROM birthwt LIMIT 5",)



out:
    printf("Closing database...\n");
    sqlite3_close(db);
    return ret;
}

