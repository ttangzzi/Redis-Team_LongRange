//redis-cli.c 

typedef struct clusterManagerNode {
    redisContext *context;
    sds name;
    char *ip;
    int port;
    int bus_port; /* cluster-port */
    uint64_t current_epoch;
    time_t ping_sent;
    time_t ping_recv;
    time_t pang_sent; /* pang_sent와 pang_recv추가 */
    time_t pang_recv;
    int flags;
    list *flags_str; /* Flags string representations */
    sds replicate;  /* Master ID if node is a slave */
    int dirty;      /* Node has changes that can be flushed */
    uint8_t slots[CLUSTER_MANAGER_SLOTS];
    int slots_count;
    int replicas_count;
    list *friends;
    sds *migrating; /* An array of sds where even strings are slots and odd
                     * strings are the destination node IDs. */
    sds *importing; /* An array of sds where even strings are slots and odd
                     * strings are the source node IDs. */
    int migrating_count; /* Length of the migrating array (migrating slots*2) */
    int importing_count; /* Length of the importing array (importing slots*2) */
    float weight;   /* Weight used by rebalance */
    int balance;    /* Used by rebalance */
} clusterManagerNode;
/*클러스터 구조체 정의. 이 구조체에 PANG 메시지를 보낸 시간과 응답을 받은 시간을 나타냄*/



static clusterManagerNode *clusterManagerNewNode(char *ip, int port, int bus_port) {
    clusterManagerNode *node = zmalloc(sizeof(*node));
    node->context = NULL;
    node->name = NULL;
    node->ip = ip;
    node->port = port;
    /* We don't need to know the bus_port, at this point this value may be wrong.
     * If it is used, it will be corrected in clusterManagerLoadInfoFromNode. */
    node->bus_port = bus_port ? bus_port : port + CLUSTER_MANAGER_PORT_INCR;
    node->current_epoch = 0;
    node->ping_sent = 0;
    node->ping_recv = 0;
    node->pang_sent = 0; /* pang 추가 */
    node->pang_recv = 0;    
    node->flags = 0;
    node->flags_str = NULL;
    node->replicate = NULL;
    node->dirty = 0;
    node->friends = NULL;
    node->migrating = NULL;
    node->importing = NULL;
    node->migrating_count = 0;
    node->importing_count = 0;
    node->replicas_count = 0;
    node->weight = 1.0f;
    node->balance = 0;
    clusterManagerNodeResetSlots(node);
    return node;
}
/*새로운 노드를 생성하는 함수에 PANG을 추가하여 PANG메시지를 보낸 시간과 응답받은 시간을 초기화*/


static int clusterManagerNodeLoadInfo(clusterManagerNode *node, int opts,
                                      char **err) /* switch 문과 if 문 pang 추가 */
{
    redisReply *reply = CLUSTER_MANAGER_COMMAND(node, "CLUSTER NODES");
    int success = 1;
    *err = NULL;
    if (!clusterManagerCheckRedisReply(node, reply, err)) {
        success = 0;
        goto cleanup;
    }
    int getfriends = (opts & CLUSTER_MANAGER_OPT_GETFRIENDS);
    char *lines = reply->str, *p, *line;
    while ((p = strstr(lines, "\n")) != NULL) {
        *p = '\0';
        line = lines;
        lines = p + 1;
        char *name = NULL, *addr = NULL, *flags = NULL, *master_id = NULL,
             *ping_sent = NULL, *ping_recv = NULL, *pang_sent = NULL, *pang_recv = NULL, *config_epoch = NULL,  //pang 보낸 시간과 받은 시간 저장 변수 초기화   
             *link_status = NULL;
        UNUSED(link_status);
        int i = 0;
        while ((p = strchr(line, ' ')) != NULL) {
            *p = '\0';
            char *token = line;
            line = p + 1;
            switch(i++){
            case 0: name = token; break;
            case 1: addr = token; break;
            case 2: flags = token; break;
            case 3: master_id = token; break;
            case 4: ping_sent = token; break;
            case 5: ping_recv = token; break;
            case 6: config_epoch = token; break;
            case 7: link_status = token; break;
            case 8: pang_sent = token; break; /*pang 추가 후 if문 break 조건 8-> 10 수정*/
            case 9: pang_recv = token; break;
            }
            if (i == 10) break; // Slots
        }
        if (!flags) {
            success = 0;
            goto cleanup;
        }

        char *ip = NULL;
        int port = 0, bus_port = 0;
        if (addr == NULL || !parseClusterNodeAddress(addr, &ip, &port, &bus_port)) {
            fprintf(stderr, "Error: invalid CLUSTER NODES reply\n");
            success = 0;
            goto cleanup;
        }

        int myself = (strstr(flags, "myself") != NULL);
        clusterManagerNode *currentNode = NULL;
        if (myself) {
            /* bus-port could be wrong, correct it here, see clusterManagerNewNode. */
            node->bus_port = bus_port;
            node->flags |= CLUSTER_MANAGER_FLAG_MYSELF;
            currentNode = node;
            clusterManagerNodeResetSlots(node);
            if (i == 8) {
                int remaining = strlen(line);
                while (remaining > 0) {
                    p = strchr(line, ' ');
                    if (p == NULL) p = line + remaining;
                    remaining -= (p - line);

                    char *slotsdef = line;
                    *p = '\0';
                    if (remaining) {
                        line = p + 1;
                        remaining--;
                    } else line = p;
                    char *dash = NULL;
                    if (slotsdef[0] == '[') {
                        slotsdef++;
                        if ((p = strstr(slotsdef, "->-"))) { // Migrating
                            *p = '\0';
                            p += 3;
                            char *closing_bracket = strchr(p, ']');
                            if (closing_bracket) *closing_bracket = '\0';
                            sds slot = sdsnew(slotsdef);
                            sds dst = sdsnew(p);
                            node->migrating_count += 2;
                            node->migrating = zrealloc(node->migrating,
                                (node->migrating_count * sizeof(sds)));
                            node->migrating[node->migrating_count - 2] =
                                slot;
                            node->migrating[node->migrating_count - 1] =
                                dst;
                        }  else if ((p = strstr(slotsdef, "-<-"))) {//Importing
                            *p = '\0';
                            p += 3;
                            char *closing_bracket = strchr(p, ']');
                            if (closing_bracket) *closing_bracket = '\0';
                            sds slot = sdsnew(slotsdef);
                            sds src = sdsnew(p);
                            node->importing_count += 2;
                            node->importing = zrealloc(node->importing,
                                (node->importing_count * sizeof(sds)));
                            node->importing[node->importing_count - 2] =
                                slot;
                            node->importing[node->importing_count - 1] =
                                src;
                        }
                    } else if ((dash = strchr(slotsdef, '-')) != NULL) {
                        p = dash;
                        int start, stop;
                        *p = '\0';
                        start = atoi(slotsdef);
                        stop = atoi(p + 1);
                        node->slots_count += (stop - (start - 1));
                        while (start <= stop) node->slots[start++] = 1;
                    } else if (p > slotsdef) {
                        node->slots[atoi(slotsdef)] = 1;
                        node->slots_count++;
                    }
                }
            }
            node->dirty = 0;
        } else if (!getfriends) {
            if (!(node->flags & CLUSTER_MANAGER_FLAG_MYSELF)) continue;
            else break;
        } else {
            currentNode = clusterManagerNewNode(sdsnew(ip), port, bus_port);
            currentNode->flags |= CLUSTER_MANAGER_FLAG_FRIEND;
            if (node->friends == NULL) node->friends = listCreate();
            listAddNodeTail(node->friends, currentNode);
        }
        if (name != NULL) {
            if (currentNode->name) sdsfree(currentNode->name);
            currentNode->name = sdsnew(name);
        }
        if (currentNode->flags_str != NULL)
            freeClusterManagerNodeFlags(currentNode->flags_str);
        currentNode->flags_str = listCreate();
        int flag_len;
        while ((flag_len = strlen(flags)) > 0) {
            sds flag = NULL;
            char *fp = strchr(flags, ',');
            if (fp) {
                *fp = '\0';
                flag = sdsnew(flags);
                flags = fp + 1;
            } else {
                flag = sdsnew(flags);
                flags += flag_len;
            }
            if (strcmp(flag, "noaddr") == 0)
                currentNode->flags |= CLUSTER_MANAGER_FLAG_NOADDR;
            else if (strcmp(flag, "disconnected") == 0)
                currentNode->flags |= CLUSTER_MANAGER_FLAG_DISCONNECT;
            else if (strcmp(flag, "fail") == 0)
                currentNode->flags |= CLUSTER_MANAGER_FLAG_FAIL;
            else if (strcmp(flag, "slave") == 0) {
                currentNode->flags |= CLUSTER_MANAGER_FLAG_SLAVE;
                if (master_id != NULL) {
                    if (currentNode->replicate) sdsfree(currentNode->replicate);
                    currentNode->replicate = sdsnew(master_id);
                }
            }
            listAddNodeTail(currentNode->flags_str, flag);
        }
        if (config_epoch != NULL)
            currentNode->current_epoch = atoll(config_epoch);
        if (ping_sent != NULL) currentNode->ping_sent = atoll(ping_sent);
        if (ping_recv != NULL) currentNode->ping_recv = atoll(ping_recv);
        if (pang_sent != NULL) currentNode->pang_sent = atoll(pang_sent); // pang 메시지 보낸 시간 유효 시 해당 값을 currentNode의 pang_sent 멤버에 저장
        if (pang_recv != NULL) currentNode->pang_recv = atoll(pang_recv); // 응답을 받은 시간이 유효하면 해당 값을 currentNode의 pang_recv 멤버에 저장
        if (!getfriends && myself) break;
    }
cleanup:
    if (reply) freeReplyObject(reply);
    return success;
}
/*클러스터 노드 정보를 로드하는 함수에 PANG을 추가. 클러스터 노드의 정보를 가져와서 위에서 정의한 clusterManagerNode 구조체에 저장 
pang 명령어 sent와 recv에 대해 값을 채워넣고 이 값으로 클러스터 노드 간의 통신을 파악 */
