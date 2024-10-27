#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <C:\Program Files\PostgreSQL\16\include\libpq-fe.h>

struct Player{
    int player_id;
    char fullname[50];
    char gender;
    char date_of_birth[11];
    int age;
    char country[50];
    char password[50];
    unsigned char *avatar;
    float balance;
    int rank;
    char registration_date[11];
};

struct Ranking {
    int rank;
    int player_id;
};

struct Friend {
    int friendship_id;
    int player1_id;
    int player2_id;
    char friend_time[11];   // the date become friend
};

struct Room {
    int room_id;
    int min_player;
    int max_player;
    int host_id;
    char status[10];
};

struct Match {
    int match_id;
    int room_id;
    int winner;
    char status[9];
    char match_date[11];
    char start_time[9];
    char end_time[9];
    unsigned char *progress;
};

struct Host {
    int host_id;
};

struct Seat {
    int seat_number;
    int player_id;
    int room_id;
    char sit[4];    // yes or no
};

struct History {
    int match_id;
};

struct Ranking getPlayerRank(PGconn *conn, int player_id) {
    struct Ranking player;
    char query[256];
    snprintf(query, sizeof(query), "SELECT * FROM ranking WHERE player_id = %d", player_id);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res)!=PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);

        player.rank = -1;
        return player;
    }

    player.player_id = atoi(PQgetvalue(res,0,1));
    player.rank = atoi(Pqgetvalue(res,0,0));

    PQclear(res);
    return player;
}

struct Player getPlayerInfo(PGconn *conn, int player_id) {
    char query[256];
    struct Player player;
    snprintf(query, sizeof(query), "SELECT * FROM player WHERE player_id = %d", player_id);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res)!=PGRES_TUPLES_OK) {
        fprintf(stderr, "No data retrieved: %s\n", PQerrorMessage(conn));
        PQclear(res);

        player.player_id = -1;
        return player;
    }

    player.player_id = atoi(PQgetvalue(res,0,0));
    strncpy(player.fullname, PQgetvalue(res,0,1), sizeof(player.fullname)-1); // -1 is just to make sure there a space for null pointer
    player.gender = PQgetvalue(res,0,2)[0];
    strncpy(player.date_of_birth, PQgetvalue(res,0,3), sizeof(player.date_of_birth)-1);
    player.age = atoi(PQgetvalue(res,0,4));
    strncpy(player.country, PQgetvalue(res,0,5), sizeof(player.country)-1);
    strncpy(player.password, PQgetvalue(res,0,6), sizeof(player.password)-1);

    // avatar as binary data
    int avatar_length = PQgetlength(res,0,7);
    player.avatar = malloc(avatar_length);
    if (player.avatar!=NULL) {
        memcpy(player.avatar, PQgetvalue(res,0,7), avatar_length); // copy a specific number of bytes
    }

    player.balance = atof(PQgetvalue(res,0,8));
    player.rank = atoi(PQgetvalue(res,0,9));
    strncpy(player.registration_date, PQgetvalue(res,0,10), sizeof(player.registration_date)-1);

    PQclear(res);
    return player;
}

struct Match getMatchInfo(PGconn *conn, int match_id) {
    char query[256];
    struct Match match;
    snprintf(query, sizeof(query), "SELECT * FROM match WHERE match_id = %d", match_id);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus!=PGRES_TUPLES_OK) {
        fprintf(stderr, "Retrieving match failed: %s", PQerrorMessage(conn));
        PQclear(res);

        match.match_id = -1;
        return match;
    }

    match.match_id = atoi(PQgetvalue(res,0,0));
    match.room_id = atoi(PQgetvalue(res,0,1));
    match.winner = atoi(PQgetvalue(res,0,2));
    strncpy(match.status, PQgetvalue(res,0,3), sizeof(match.status)-1);
    strncpy(match.match_date, PQgetvalue(res,0,4), sizeof(match.match_date)-1);
    strncpy(match.start_time, PQgetvalue(res,0,5), sizeof(match.start_time)-1);
    strncpy(match.end_time, PQgetvalue(res,0,6), sizeof(match.end_time)-1);

    int progress_length = PQgetlength(res,0,7);
    match.progress = malloc(progress_length);
    if (match.progress!=NULL) {
        memcpy(match.progress, PQgetvalue(res,0,7), progress_length);
    }

    PQclear(res);
    return match;
};

void updateMatchStatus (PGconn *conn, int match_id, char *new_status) {
    char query[256];

    snprintf(query, sizeof(query), "UPDATE match SET status = %s WHERE match_id = %d", match_id);
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res)!=PGRES_COMMAND_OK) {
        fprintf(stderr, "Update match status failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    printf("Match status updated successfully.\n");
    PQclear(res);
}

void createMatch(PGconn *conn, int match_id, int room_id, int winner, char *match_date, char *start_time, char *end_time) {
    char query[256];
    // create match means 'status = progress' automatically
    snprintf(query, sizeof(query), "INSERT INTO match (match_id, room_id, winner, status, match_date, start_time, end_time, progress) "
                                   "VALUES (%d, %d, %d, %s, %s, %s, %s, %s)",
                                   match_id, room_id, winner, "progress", match_date, start_time, end_time, "");
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res)!=PGRES_COMMAND_OK) {
        fprintf(stderr, "Creating match failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    printf("Match created successfully.\n");
    PQclear(res);
}

void endMatch(PGconn *conn, int match_id, unsigned char *progress) {
    char query[256];

    snprintf(query, sizeof(query), "UPDATE match SET status = 'finish', progress = '%s'  WHERE match_id = %d", progress, match_id);
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res)!=PGRES_COMMAND_OK) {
        fprintf(stderr, "Ending match failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    printf("Ending match successfully.\n");
    PQclear(res);
}

void update_rank(PGconn *conn, int player_id, int new_rank) {
    char query[256];

    snprintf(query, sizeof(query), "UPDATE ranking SET rank = %d WHERE player_id = %d", new_rank, player_id);
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res)!=PGRES_COMMAND_OK) {
        fprintf(stderr, "Update failed in raking table: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    PQclear(res);

    snprintf(query, sizeof(query), "UPDATE player SET rank = %d WHERE player_id = %d", new_rank, player_id);
    res = PQexec(conn, query);
    if (PQresultStatus(res)!=PGRES_COMMAND_OK) {
        fprintf(stderr, "Update failed in player table: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    printf("Rank updated successfully.\n");
    PQclear(res);
}

struct Ranking *leaderboard(PGconn *conn) {
    char query[256];
    snprintf(query, sizeof(query), "SELECT * FROM ranking ORDER BY rank ASC");

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res)!=PGRES_TUPLES_OK) {
        fprintf(stderr, "Retrieving leaderboard failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }

    int num_rows = PQntuples(res);
    struct Ranking *leaderboard = malloc(num_rows*sizeof(struct Ranking));
    if (leaderboard==NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        PQclear(res);
        return NULL;
    }

    for (int i=0;i<num_rows;i++) {
        leaderboard[i].rank = atoi(PQgetvalue(res,i,0));
        leaderboard[i].player_id = atoi(PQgetvalue(res,i,1));
    }

    PQclear(res);
    return leaderboard;
}

int main() {
    char *conninfo = "dbname=cardio user='' password=vietzanh204 host=localhost port=5432";

//  create connection
    PGconn *conn = PQconnectdb(conninfo);

//  check if connection is successful
    if (PQstatus(conn)!=CONNECTION_OK) {
//      if fail, print error message and finish connection
        printf("Connection to database fails: %s\n", PQerrorMessage(conn));

//      finish connection
        PQfinish(conn);

        exit(1);
    }

//  if successful
    printf("Connection established\n");
    printf("Port: %s\n", PQport(conn));
    printf("Host: %s\n",PQhost(conn));
    printf("Database name: %s\n", PQdb(conn));

    PQfinish(conn);

    return 0;
}
