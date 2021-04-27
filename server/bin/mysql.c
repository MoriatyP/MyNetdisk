#include "../include/server.h"

int mysql_connect(MYSQL **conn, char *mysql_path){
    time_t now;
    config_t config[MAX_CONFIG];
    int config_num = read_config(mysql_path, config);
    char server[CONFIG_LEN] = {0}, user[CONFIG_LEN] = {0}, password[CONFIG_LEN] = {0}, database[CONFIG_LEN] = {0};
    get_config_data(config, config_num, "server", server);
    get_config_data(config, config_num, "user", user);
    get_config_data(config, config_num, "password", password);
    get_config_data(config, config_num, "database", database);
    *conn = mysql_init(NULL);
    if (!mysql_real_connect(*conn, server, user, password, database, 0, NULL, 0)){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("Error connecting to database:%s\n", mysql_error(*conn));
        return -1;
    }else{
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql connected...\n");
        return 0;
    }
}

MYSQL_RES *mysql_select(MYSQL *conn, const char *table, const char *field, const char *data){
    time_t now;
    MYSQL_RES *res = NULL;
    char query[SQL_LEN];
    sprintf(query, "SELECT * FROM %s WHERE %s = '%s'", table, field, data);
    time(&now);
    printf("%s: ", ctime(&now));
    printf("mysql> %s\n", query);
    int ret = mysql_query(conn, query);
    if (ret){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql> Error making query: %s\n", mysql_error(conn));
        return NULL;
    }else{
        res = mysql_store_result(conn);
        if (mysql_num_rows(res) == 0){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("mysql> empty set\n");
            mysql_free_result(res);
            return NULL;
        }
        return res;
    }
}

MYSQL_RES *mysql_select_int(MYSQL *conn, const char *table, const char *field, int data){
    time_t now;
    MYSQL_RES *res = NULL;
    char query[SQL_LEN];
    sprintf(query, "SELECT * FROM %s WHERE %s = %d", table, field, data);
    time(&now);
    printf("%s: ", ctime(&now));
    printf("mysql> %s\n", query);
    int ret = mysql_query(conn, query);
    if (ret){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql> Error making query: %s\n", mysql_error(conn));
        return NULL;
    }else{
        res = mysql_store_result(conn);
        if (mysql_num_rows(res) == 0){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("mysql> empty set\n");
            mysql_free_result(res);
            return NULL;
        }
        return res;
    }
}

MYSQL_RES *mysql_select_two(MYSQL *conn, const char *table, const char *field1, const char *data1, const char *field2, int data2){
    time_t now;
    MYSQL_RES *res = NULL;
    char query[SQL_LEN];
    sprintf(query, "SELECT * FROM %s WHERE %s = '%s' and %s=%d", table, field1, data1, field2, data2);
    time(&now);
    printf("%s: ", ctime(&now));
    printf("mysql> %s\n", query);
    int ret = mysql_query(conn, query);
    if (ret){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql> Error making query: %s\n", mysql_error(conn));
        return NULL;
    }else{
        res = mysql_store_result(conn);
        if (mysql_num_rows(res) == 0){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("mysql> empty set\n");
            mysql_free_result(res);
            return NULL;
        }
        return res;
    }
}

MYSQL_RES *mysql_select_dir(MYSQL *conn, const char *table, const char *field1, int data1, const char *field2, int data2){
    time_t now;
    MYSQL_RES *res = NULL;
    char query[SQL_LEN];
    sprintf(query, "SELECT * FROM %s WHERE %s = %d and %s = %d and filesize = 0", table, field1, data1, field2, data2);
    time(&now);
    printf("%s: ", ctime(&now));
    printf("mysql> %s\n", query);
    int ret = mysql_query(conn, query);
    if (ret){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql> Error making query: %s\n", mysql_error(conn));
        return NULL;
    }else{
        res = mysql_store_result(conn);
        if (mysql_num_rows(res) == 0){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("mysql> empty set\n");
            mysql_free_result(res);
            return NULL;
        }
        return res;
    }
}

int mysql_insert_user(MYSQL *conn, const char *user_name, const char *password, const char *salt){
    time_t now;
    int ret;
    char query[SQL_LEN];
    sprintf(query, "INSERT INTO user VALUES (default, '%s', '%s', '%s')", user_name, password, salt);
    time(&now);
    printf("%s: ", ctime(&now));
    printf("mysql> %s\n", query);
    ret = mysql_query(conn, query);
    if (ret){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql> Error making query: %s\n", mysql_error(conn));
        return -1;
    }else{
        return 0;
    }
}

int mysql_insert_dir(MYSQL *conn, int pre_id, int user_id, const char *dir_name){
    time_t now;
    int ret;
    char query[SQL_LEN];
    sprintf(query, "INSERT INTO files VALUES (default,%d,%d,'%s','%s',0,default,default)", pre_id, user_id, dir_name, dir_name);
    time(&now);
    printf("%s: ", ctime(&now));
    printf("mysql> %s\n", query);
    ret = mysql_query(conn, query);
    if (ret){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql> Error making query: %s\n", mysql_error(conn));
        return -1;
    }else{
        return 0;
    }
}

int mysql_insert_file(MYSQL *conn, int pre_id, int user_id, const char *filename, const char *real_filename, off_t filesize, const char *md5sum){
    time_t now;
    int ret;
    char query[SQL_LEN];
    sprintf(query, "INSERT INTO files VALUES (default,%d,%d,'%s','%s',%ld,'%s',default)", pre_id, user_id, filename, real_filename, filesize, md5sum);
    time(&now);
    printf("%s: ", ctime(&now));
    printf("mysql> %s\n", query);
    ret = mysql_query(conn, query);
    if (ret){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql> Error making query: %s\n", mysql_error(conn));
        return -1;
    }else{
        return 0;
    }
}

int mysql_delete_user(MYSQL *conn, const char *user_name, const char *password){
    time_t now;
    char query[SQL_LEN] = {0};
    sprintf(query, "DELETE FROM user WHERE user_name = '%s' AND password = '%s'", user_name, password);
    time(&now);
    printf("%s: ", ctime(&now));
    printf("mysql> %s\n", query);
    int ret = mysql_query(conn, query);
    if (ret){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql> Error making query: %s\n", mysql_error(conn));
        return -1;
    }
    return 0;
}

int mysql_delete_file(MYSQL *conn, int file_id){
    time_t now;
    char query[SQL_LEN] = {0};
    sprintf(query, "DELETE FROM files WHERE file_id = %d", file_id);
    time(&now);
    printf("%s: ", ctime(&now));
    printf("mysql> %s\n", query);
    int ret = mysql_query(conn, query);
    if (ret){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql> Error making query: %s\n", mysql_error(conn));
        return -1;
    }
    return 0;
}
