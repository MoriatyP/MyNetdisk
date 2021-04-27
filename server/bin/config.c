#include "../include/server.h"

int read_config(char *path, pconfig_t config){
    FILE *fp = fopen(path, "r");
    char buf[CONFIG_LEN] = {0};
    int i, j, k;
    int tag = 0;
    while(fgets(buf, sizeof(buf), fp) != NULL){
        if(buf[0] == '#' || buf[0] == '\n'){
            continue;
        }
        else{
            i = 0;
            while(buf[i] != '\0'){
                if(buf[i] == ' '){
                    config[tag].key = (char*)malloc(i + 1);
                    strncpy(config[tag].key, buf, i);
                    config[tag].key[i] = '\0';
                }
                else if(buf[i] == '\n'){
                    j = strlen(config[tag].key);
                    config[tag].data = (char*)malloc(i - j);
                    for (k = 0; k < i - j; k++){
                        config[tag].data[k] = buf[k + j + 1];
                    }
                    config[tag].data[k - 1] = '\0';
                }
                i++;
            }
        }    
        tag++;
    }
    return tag;
}

int get_config_data(pconfig_t config, int confignum, char *key, char *data){
    for(int i = 0; i < confignum; i++){
        if(strcmp(config[i].key, key) == 0){
            strcpy(data, config[i].data);
            return 0;
        }
    }
    return -1;
}

int get_server_config(pconfig_t config, int config_num, server_config *message){
    int ret;
    time_t now;
    ret = get_config_data(config, config_num, "ip", message->ipstr);
    ERROR_CHECK(ret, -1, "get_ip");
    ret = get_config_data(config, config_num, "port", message->portstr);
    ERROR_CHECK(ret, -1, "get_port");
    ret = get_config_data(config, config_num, "threadnum1", message->threadnum1str);
    ERROR_CHECK(ret, -1, "get_threadnum1");
    ret = get_config_data(config, config_num, "threadnum2", message->threadnum2str);
    ERROR_CHECK(ret, -1, "get_threadnum2");
    ret = get_config_data(config, config_num, "quesize", message->quesizestr);
    ERROR_CHECK(ret, -1, "get_quesize");
    ret = get_config_data(config, config_num, "max_clients", message->maxclientstr);
    ERROR_CHECK(ret, -1, "get_max_clients");
    ret = get_config_data(config, config_num, "mysql_path", message->mysqlpathstr);
    ERROR_CHECK(ret, -1, "get_mysql_path");
    ret = get_config_data(config, config_num, "log_path", message->logpathstr);
    ERROR_CHECK(ret, -1, "get_log_path");
    return 0;
}
